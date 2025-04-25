#include "token.hpp"
#include <fstream>
#include <iostream>
#include <string>
int main(int argc, char *argv[]) {
  std::ifstream stream{argv[0]};
  std::string content;

  stream >> content;
  std::cout << content;
  Token token{TokenType::STRING};

  return 0;
}
