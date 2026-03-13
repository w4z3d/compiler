#include "alloc/arena.hpp"
#include "analysis/liveness.hpp"
#include "analysis/symbol.hpp"
#include "analysis/type_check.hpp"
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
#include "type/source_type_registry.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

void clear_last_line() { std::cout << "\033[A\033[2K" << std::flush; }

std::string find_runtime(const std::string &compiler_path) {
  // Runtime liegt neben dem Compiler Binary
  auto compiler_dir = std::filesystem::path(compiler_path).parent_path();
  auto runtime = compiler_dir / "runtime.o";
  if (std::filesystem::exists(runtime))
    return runtime.string();

  // Fallback: aktuelles Verzeichnis
  if (std::filesystem::exists("runtime.o"))
    return "runtime.o";

  // Fallback: relativ zum Source
  if (std::filesystem::exists("runtime/runtime.o"))
    return "runtime/runtime.o";

  return "";
}

int main(int argc, char *argv[]) {
  const auto file = io::read_file(argv[1]);

  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);
  auto lexer = std::make_unique<Lexer>(file.name, file.content);
  auto parser = std::make_unique<Parser>(*lexer, diagnostics, source_manager);

  printf("[+] Parsing file\n");
  const auto unit{parser->parse_translation_unit()};

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 42;
  }

  const auto symbol_table = std::make_shared<SymbolTable>();
  source_type::TypeRegistry source_types{};
  type_check::TypeVisitor type_check_visitor{diagnostics, source_manager,
                                             symbol_table, source_types};

  clear_last_line();
  printf("[+] Performing type checking\n");
  unit->accept(type_check_visitor);

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 7;
  }

  hir::Module module{};
  HIRBuilderVisitor hir_visitor{module, symbol_table, diagnostics,
                                source_types};
  clear_last_line();
  printf("[+] Emitting HIR\n");
  unit->accept(hir_visitor);

  std::cout << module.to_string() << std::endl;
  OptPipeline opt{};

  clear_last_line();
  printf("[+] Optimizing HIR\n");
  // for (auto &function : module.functions) {
  //   opt.optimize(function.get());
  // }
  lir::Module lir_module{};
  LIRLowering lowering{lir_module, aarch64::target};

  clear_last_line();
  printf("[+] lowering to lir\n");

  lowering.lower_module(module);
  std::cout << lir_module.to_string() << std::endl;

  clear_last_line();
  printf("[+] Performing Register Allocation\n");
  for (auto *func : lir_module.functions) {
    auto info = liveness::compute_liveness(func);
    UndirectedGraph graph(info.num_vregs);
    liveness::build_interference_graph(func, info, graph);

    auto coalesce_info =
        regalloc::coalesce(func, graph, aarch64::target.num_allocatable);

    auto precolored = precolor(func);

    auto coloring = graph.color(precolored, info.num_vregs);

    for (auto &[removed, kept] : coalesce_info.representative) {
      unsigned root = coalesce_info.find(removed);
      if (coloring.contains(root)) {
        coloring[removed] = coloring[root];
      }
    }

    regalloc::rewrite_registers(func, coloring);
  }

  aarch64::AsmEmitter emitter(aarch64::target);
  clear_last_line();
  printf("[+] Emitting Assembly\n");
  std::string asm_output = emitter.emit_module(&lir_module);

  // Write to file
  std::ofstream out("output.s");
  out << asm_output;
  out.close();
  std::string runtime_path = find_runtime(argv[0]);
  if (runtime_path.empty()) {
    std::cerr << "Error: cannot find runtime.o\n";
    std::cerr << "Run: clang -c runtime/runtime.c -o runtime.o\n";
    return 1;
  }

  const std::string link_cmd = "ld -o output output.o " + runtime_path +
                               " -lSystem -syslibroot $(xcrun --sdk macosx "
                               "--show-sdk-path) -arch arm64";
  // Assemble and link
  system("as -o output.o output.s");
  system(link_cmd.c_str());
  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 7;
  }

  return 0;
}
