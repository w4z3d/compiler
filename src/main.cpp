#include "analysis/liveness.hpp"
#include "analysis/semantics.hpp"
#include "analysis/symbol.hpp"
#include "analysis/type_check.hpp"
#include "args.hpp"
#include "codegen/aarch64/aarch64_defs.hpp"
#include "codegen/aarch64/aarch64_emit.hpp"
#include "codegen/aarch64/precolor_info.hpp"
#include "codegen/regalloc.hpp"
#include "defs/ast.hpp"
#include "defs/ast_printer.hpp"
#include "graph_coloring/graph_coloring.hpp"
#include "hir/hir.hpp"
#include "hir/hir_builder_visitor.hpp"
#include "hir/opt/optimization_pipeline.hpp"
#include "io/io.hpp"
#include "lexer/lexer.hpp"
#include "lir/lir.hpp"
#include "lir/lir_lowering.hpp"
#include "parser/parser.hpp"
#include "report/report_builder.hpp"
#include <fstream>
#include <iostream>
#include <memory>

void clear_last_line() { std::cout << "\033[A\033[2K" << std::flush; }

static void log(const CompilerOptions &opts, const char *msg) {
  if (opts.verbose)
    std::cerr << "[+] " << msg << "\n";
}

static int run_command(const std::string &cmd) {
  int ret = system(cmd.c_str());
  if (ret != 0) {
    std::cerr << "Error: command failed: " << cmd << "\n";
  }
  return ret;
}

int main(int argc, char *argv[]) {
  auto opts_opt = parse_args(argc, argv);
  if (!opts_opt)
    return 1;
  auto &opts = *opts_opt;

  const auto file = io::read_file(opts.input_file);
  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);

  log(opts, "Parsing");
  auto lexer = std::make_unique<Lexer>(file.name, file.content);
  auto parser = std::make_unique<Parser>(*lexer, diagnostics, source_manager);
  const auto unit = parser->parse_translation_unit();

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 1;
  }

  if (opts.dump_ast) {
    ClangStylePrintVisitor printer{};
    unit->accept(printer);
    std::cout << printer.get_content() << std::endl;
  }

  log(opts, "Semantic analysis");
  const auto symbol_table = std::make_shared<SymbolTable>();
  semantic::SemanticVisitor semantic_visitor{diagnostics, source_manager,
                                             symbol_table};
  unit->accept(semantic_visitor);

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 1;
  }

  log(opts, "Type checking");
  type_check::TypeVisitor type_check_visitor{diagnostics, source_manager,
                                             symbol_table};
  unit->accept(type_check_visitor);

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 1;
  }

  log(opts, "HIR generation");
  hir::Module hir_module{};
  HIRBuilderVisitor hir_visitor{hir_module, symbol_table, diagnostics};
  unit->accept(hir_visitor);

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 1;
  }

  if (opts.emit_hir)
    std::cout << hir_module.to_string() << std::endl;

  if (opts.opt_level >= 1) {
    log(opts, "Optimizing HIR");
    OptPipeline opt{};
    for (auto &function : hir_module.functions) {
      opt.optimize(function.get());
    }

    if (opts.emit_hir) {
      std::cout << "; === After optimization ===\n"
                << hir_module.to_string() << std::endl;
    }
  }

  log(opts, "Lowering to LIR");
  lir::Module lir_module{};
  LIRLowering lowering{lir_module, aarch64::target};
  lowering.lower_module(hir_module);

  if (opts.emit_lir)
    std::cout << lir_module.to_string() << std::endl;

  log(opts, "Register allocation");

  for (auto *func : lir_module.functions) {
    if (func->is_extern)
      continue;

    // Liveness
    auto info = liveness::compute_liveness(func);

    // Interference graph
    UndirectedGraph graph(info.num_vregs);
    liveness::build_interference_graph(func, info, graph);

    if (opts.dump_interference)
      std::cout << func->name << ":" << graph.to_string() << std::endl;

    // Coalesce
    auto coalesce_info =
        regalloc::coalesce(func, graph, aarch64::target.num_allocatable);

    auto precolored = precolor(func);

    auto coloring = graph.color(precolored, info.num_vregs);

    for (auto &[removed, kept] : coalesce_info.representative) {
      unsigned root = coalesce_info.find(removed);
      if (coloring.contains(root))
        coloring[removed] = coloring[root];
    }

    regalloc::rewrite_registers(func, coloring);
  }

  if (opts.emit_lir) {
    std::cout << "; === After register allocation ===\n"
              << lir_module.to_string() << std::endl;
  }

  log(opts, "Emitting assembly");
  aarch64::AsmEmitter emitter(aarch64::target);
  std::string asm_output = emitter.emit_module(&lir_module);

  if (opts.emit_asm)
    std::cout << asm_output << std::endl;

  std::string asm_file = opts.output_file + ".s";
  std::string obj_file = opts.output_file + ".o";

  {
    std::ofstream out(asm_file);
    if (!out) {
      std::cerr << "Error: cannot write to " << asm_file << "\n";
      return 1;
    }
    out << asm_output;
  }

  if (opts.emit == CompilerOptions::EmitFormat::Assembly) {
    log(opts, "Done (assembly)");
    return 0;
  }

  log(opts, "Assembling");
  if (run_command("as -o " + obj_file + " " + asm_file) != 0)
    return 1;

  if (opts.emit == CompilerOptions::EmitFormat::Object) {
    log(opts, "Done (object)");
    return 0;
  }

  log(opts, "Linking");
  std::string link_cmd =
      "ld -o " + opts.output_file + " " + obj_file +
      " -lSystem -syslibroot $(xcrun --sdk macosx --show-sdk-path) -arch arm64";

  if (run_command(link_cmd) != 0)
    return 1;

  log(opts, "Done");
  return 0;
}
