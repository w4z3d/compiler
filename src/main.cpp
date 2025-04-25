#include "token.hpp"
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
int main(int argc, char *argv[]) {

  token::Token tok{token::TokenType::STRING};
  auto type = tok.get_token_type();

  spdlog::log(spdlog::level::info, "Test Hello World!");

  return 0;
}
