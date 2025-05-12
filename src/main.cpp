#include "analysis/semantics.hpp"
#include "defs/ast.hpp"
#include "defs/ast_printer.hpp"
#include "io/io.hpp"
#include "ir/ir_builder.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "report/report_builder.hpp"
#include "spdlog/cfg/env.h"
#include <iostream>
#include <memory>

int main(int argc, char *argv[]) {

  spdlog::cfg::load_env_levels();
  // spdlog::info("Start");
  const auto file = io::read_file(argv[1]);
  // std::cout << file.content << std::endl;

  const auto diagnostics = std::make_shared<DiagnosticEmitter>();
  const auto source_manager =
      std::make_shared<SourceManager>(file.content, file.name);
  Lexer lexer{file.name, file.content};
  Parser parser{lexer, diagnostics, source_manager};

  // ClangStylePrintVisitor visitor{};
  const auto unit{parser.parse_translation_unit()};
  // unit->accept(visitor);
  // std::cout << visitor.get_content() << std::endl;

  semantic::SemanticVisitor semantic_visitor{diagnostics, source_manager};
  unit->accept(semantic_visitor);

  IntermediateRepresentation representation{};
  IRBuilder builder{representation, diagnostics, source_manager};
  unit->accept(builder);

  std::cout << representation.to_string() << std::endl;


  diagnostics->print_all();
  // spdlog::info("End");
  system("pause");
  return 0;
}
