#include "io/io.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "spdlog/cfg/env.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[]) {

  spdlog::cfg::load_env_levels();

  const auto file = io::read_file(argv[1]);
  std::cout << file.content << std::endl;
  Lexer lexer{file.name, file.content};
  Parser parser{lexer};

  const auto unit{parser.parse_translation_unit()};
  spdlog::log(spdlog::level::info, "Unit: {}",
              unit->getDeclarations()[0]->getName());

  return 0;
}
