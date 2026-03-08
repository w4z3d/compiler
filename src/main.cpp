#include "analysis/liveness.hpp"
#include "analysis/semantics.hpp"
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
#include <fstream>
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {
  const auto file = io::read_file(argv[1]);

  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);
  auto lexer = std::make_unique<Lexer>(file.name, file.content);
  auto parser = std::make_unique<Parser>(*lexer, diagnostics, source_manager);

  const auto unit{parser->parse_translation_unit()};

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 42;
  }

  const auto symbol_table = std::make_shared<SymbolTable>();
  semantic::SemanticVisitor semantic_visitor{diagnostics, source_manager,
                                             symbol_table};
  unit->accept(semantic_visitor);
  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 7;
  }

  type_check::TypeVisitor type_check_visitor{diagnostics, source_manager,
                                             symbol_table};
  unit->accept(type_check_visitor);

  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 7;
  }

  hir::Module module{};
  HIRBuilderVisitor hir_visitor{module, symbol_table, diagnostics};
  unit->accept(hir_visitor);

  OptPipeline opt{};

  std::cout << module.to_dot() << std::endl;

  // for (auto &function : module.functions) {
  //   opt.optimize(function.get());
  // }
  lir::Module lir_module{};
  LIRLowering lowering{lir_module, aarch64::target};
  lowering.lower_module(module);

  std::cout << lir_module.to_string() << std::endl;

  for (auto *func : lir_module.functions) {
    // Liveness analysis
    auto info = liveness::compute_liveness(func);

    // Build interference graph
    UndirectedGraph graph(info.num_vregs);
    liveness::build_interference_graph(func, info, graph);

    std::vector<std::pair<size_t, size_t>> precolored = precolor(func);
    // Graph coloring
    auto coloring = graph.color(precolored, info.num_vregs);

    // Rewrite vregs → physical registers and remove dead copies
    regalloc::rewrite_registers(func, coloring);
  }

  std::cout << lir_module.to_string() << std::endl;
  aarch64::AsmEmitter emitter(aarch64::target);
  std::string asm_output = emitter.emit_module(&lir_module);

  // Write to file
  std::ofstream out("output.s");
  out << asm_output;
  out.close();

  // Assemble and link
  system("as -o output.o output.s");
  system("ld -o output output.o -lSystem -syslibroot $(xcrun --sdk macosx "
         "--show-sdk-path) -arch arm64");
  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    return 7;
  }

  return 0;
}
