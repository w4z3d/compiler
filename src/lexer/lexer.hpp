#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include "../defs/token.hpp"
#include <cctype>
#include <cstddef>
#include <string_view>

class Lexer {
private:
  std::string_view input_;
  std::size_t pos_;
  void skip_whitespaces();
};

#endif // !LEXER_LEXER_H
