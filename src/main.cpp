#include "analysis/semantics.hpp"
#include "analysis/symbol.hpp"
#include "analysis/type_check.hpp"
#include "defs/ast.hpp"
#include "defs/ast_printer.hpp"
#include "hir/hir.hpp"
#include "hir/hir_builder_visitor.hpp"
#include "hir/opt/optimization_pipeline.hpp"
#include "io/io.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "report/report_builder.hpp"
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {
  const auto file = io::read_file(argv[1]);
  // std::cout << file.content << std::endl;

  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);
  auto *lexer = new Lexer{file.name, file.content};
  auto *parser = new Parser{*lexer, diagnostics, source_manager};

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

  type_check::TypeVisitor type_check_visitor{diagnostics, source_manager};
  unit->accept(type_check_visitor);

  ClangStylePrintVisitor visitor{};
  unit->accept(visitor);
  std::cout << visitor.get_content() << std::endl;

  hir::Module module{};
  HIRBuilderVisitor hir_visitor{module, symbol_table, diagnostics};
  unit->accept(hir_visitor);

  std::cout << module.to_dot() << std::endl;

  std::cout << "\n\n\n ============ Optimizing =============\n" << std::endl;

  OptPipeline::optimize(module);

  std::cout << module.to_dot() << std::endl;
  if (diagnostics->has_errors()) {
    diagnostics->print_all();
    system("pause");
    return 7;
  }

  delete parser;
  delete lexer;

  return 0;
}
