#include "analysis/semantics.hpp"
#include "defs/ast.hpp"
#include "io/io.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "spdlog/cfg/env.h"
#include <iostream>

int main(int argc, char *argv[]) {

  spdlog::cfg::load_env_levels();

  const auto file = io::read_file(argv[1]);
  std::cout << file.content << std::endl;
  Lexer lexer{file.name, file.content};
  Parser parser{lexer};

  ClangStylePrintVisitor visitor{};
  const auto unit{parser.parse_translation_unit()};
  unit->accept(visitor);
  std::cout << visitor.get_content() << std::endl;

  semantic::SemanticVisitor semantic_visitor{};
  unit->accept(semantic_visitor);

  return 0;
}
