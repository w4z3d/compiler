#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H

#include "../defs/token.hpp"
#include <cstddef>
#include <string_view>

class Lexer {
public:
  Lexer(std::string_view file_name, std::string_view source);

  token::Token next_token();
  bool eof() const;
  const std::string_view get_file_name() const { return file_name; }

private:
  std::string_view file_name;
  std::string_view source;
  std::size_t index = 0;
  int line = 1;
  int column = 1;

  char peek() const;
  char get();

  void skip_whitespace();

  token::Token lex_identifier_or_keyword();
  token::Token lex_number();
  token::Token lex_operator_or_punctuation();
  token::Token lex_string_literal();
  token::Token lex_char_literal();
};

#endif // !LEXER_LEXER_H
