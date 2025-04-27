#include "lexer.hpp"
#include <cctype>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string_view>

Lexer::Lexer(std::string_view src) : source(src) {}

char Lexer::peek() const {
  return index < source.size() ? source[index] : '\0';
}

char Lexer::get() {
  if (index >= source.size())
    return '\0';

  char c = source[index++];
  if (c == '\0') {
    line++;
    column = 1;
  } else {
    column++;
  }

  return c;
}

bool Lexer::eof() const { return index >= source.size(); }

void Lexer::skip_whitespace() {
  while (std::isspace(peek()))
    get();
}

token::Token Lexer::next_token() {
  skip_whitespace();

  if (eof()) {
    return {token::TokenKind::Eof, "EOF", line, column};
  }

  char c = peek();
  if (std::isalpha(c) || c == '_') {
    return lex_identifier_or_keyword();
  }
  if (std::isdigit(c)) {
    return lex_number();
  }
  if (c == '"') {
    return lex_string_literal();
  }
  if (c == '\'') {
    return lex_char_literal();
  }

  return lex_operator_or_punctuation();
}

token::Token Lexer::lex_identifier_or_keyword() {
  size_t start = index;
  while (std::isalnum(peek()) || peek() == '_')
    get();

  std::string_view ident = source.substr(start, index - start);

  return token::Token{token::TokenKind::Identifier, ident, line, column};
}

token::Token Lexer::lex_number() {
  spdlog::log(spdlog::level::info, "Lexing number...");
  size_t start = index;

  while (std::isdigit(peek()))
    get();

  std::string_view text = source.substr(start, index - start);

  return token::Token{token::TokenKind::Number, text, line, column};
}

token::Token Lexer::lex_string_literal() {
  size_t start = index;
  // Consume "
  get();

  while (peek() != '\"')
    get();

  // Consume "
  get();

  std::string_view text = source.substr(start, index - start);

  return token::Token{token::TokenKind::String, text, line, column};
}

token::Token Lexer::lex_char_literal() {
  size_t start = index;
  bool is_invalid = false;

  // Consume initial
  get();

  if (eof()) {
    is_invalid = true;
  } else {
    if (peek() != '\'') {
      is_invalid = true;
    } else {
      get();
    }
  }

  get();

  std::string_view text = source.substr(start, index - start);

  return token::Token{token::TokenKind::Char, text, line, column, is_invalid};
}

token::Token Lexer::lex_operator_or_punctuation() {
  std::string_view text = source.substr(index, 1);
  char c = get();
  switch (c) {
  case '+':
    return token::Token{token::TokenKind::Plus, text, line, column};
  case '-':
    return token::Token{token::TokenKind::Minus, text, line, column};
  case '(':
    return token::Token{token::TokenKind::LParen, text, line, column};
  case ')':
    return token::Token{token::TokenKind::RParen, text, line, column};
  case '{':
    return token::Token{token::TokenKind::LBrace, text, line, column};
  case '}':
    return token::Token{token::TokenKind::RBrace, text, line, column};
  default:
    return token::Token{token::TokenKind::Unsupported, text, line, column};
  };
}
