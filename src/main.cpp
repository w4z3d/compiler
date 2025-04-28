#include "io/io.hpp"
#include "lexer/lexer.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[]) {
  const auto file = io::read_file(argv[1]);

  Lexer lexer{file.name, file.content};

  while (!lexer.eof()) {
    const auto token{lexer.next_token()};
    spdlog::log(spdlog::level::info, "{}", std::string{token});
  }

  return 0;
}
