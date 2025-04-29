#include "lexer.hpp"
#include <cctype>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string_view>
#include <tuple>

Lexer::Lexer(std::string_view file_name, std::string_view src)
    : file_name(file_name), source(src) {}

char Lexer::peek() const {
  return index < source.size() ? source[index] : '\0';
}

char Lexer::get() {
  if (index >= source.size())
    return '\0';

  char c = source[index++];
  if (c == '\n') {
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
    return {token::TokenKind::Eof, "EOF",
            token::Span{file_name, std::make_tuple(line, column),
                        std::make_tuple(line, column)}};
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
  size_t start_index = index;
  const auto start{std::make_tuple(line, column)};

  while (std::isalnum(peek()) || peek() == '_')
    get();

  std::string_view ident = source.substr(start_index, index - start_index);

  const auto end{std::make_tuple(line, column)};
  token::Span span{file_name, start, end};

  // check if identifier is keyword
  if (token::keywordTable.contains(ident)) {
    return token::Token{token::keywordTable.at(ident), ident, span};
  }
  return token::Token{token::TokenKind::Identifier, ident, span};
}

token::Token Lexer::lex_number() {
  size_t start_index = index;

  const auto start{std::make_tuple(line, column)};
  while (std::isdigit(peek()))
    get();

  std::string_view text = source.substr(start_index, index - start_index);

  const auto end{std::make_tuple(line, column)};
  token::Span span{file_name, start, end};
  return token::Token{token::TokenKind::Number, text, span};
}

token::Token Lexer::lex_string_literal() {
  size_t start_index = index;
  const auto start{std::make_tuple(line, column)};
  // Consume "
  get();

  while (peek() != '\"')
    get();

  // Consume "
  get();

  std::string_view text = source.substr(start_index, index - start_index);

  const auto end{std::make_tuple(line, column)};
  token::Span span{file_name, start, end};
  return token::Token{token::TokenKind::String, text, span};
}

token::Token Lexer::lex_char_literal() {
  size_t start_index = index;
  const auto start{std::make_tuple(line, column)};
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

  std::string_view text = source.substr(start_index, index - start_index);

  const auto end{std::make_tuple(line, column)};
  token::Span span{file_name, start, end};
  return token::Token{token::TokenKind::Char, text, span, is_invalid};
}

// TODO: Support doubled punctuation (>>, <<, <=, >=, ...)
token::Token Lexer::lex_operator_or_punctuation() {
  std::string_view text = source.substr(index, 1);
  const auto start{std::make_tuple(line, column)};
  const auto end{std::make_tuple(line, column + 1)};
  const token::Span span{file_name, start, end};

  char c = get();
  switch (c) {
  case '+':
    return token::Token{token::TokenKind::Plus, text, span};
  case '-':
    return token::Token{token::TokenKind::Minus, text, span};
  case '(':
    return token::Token{token::TokenKind::LParen, text, span};
  case ')':
    return token::Token{token::TokenKind::RParen, text, span};
  case '{':
    return token::Token{token::TokenKind::LBrace, text, span};
  case '}':
    return token::Token{token::TokenKind::RBrace, text, span};
  case '[':
    return token::Token{token::TokenKind::LBracket, text, span};
  case ']':
    return token::Token{token::TokenKind::RBracket, text, span};
  case '/':
    return token::Token{token::TokenKind::Slash, text, span};
  case '*':
    return token::Token{token::TokenKind::Asterisk, text, span};
  case '#':
    return token::Token{token::TokenKind::Hash, text, span};
  case '<':
    return token::Token{token::TokenKind::LAngleBracket, text, span};
  case '>':
    return token::Token{token::TokenKind::RAngleBracket, text, span};
  case ',':
    return token::Token{token::TokenKind::Comma, text, span};
  case ';':
    return token::Token{token::TokenKind::Semi, text, span};
  case '@':
    return token::Token{token::TokenKind::At, text, span};
  case '\\':
    return token::Token{token::TokenKind::BackSlash, text, span};
  case '=':
    return token::Token{token::TokenKind::Equals, text, span};
  case '!':
    return token::Token{token::TokenKind::Exclamation, text, span};
  case ':':
    return token::Token{token::TokenKind::Colon, text, span};
  case '.':
    return token::Token{token::TokenKind::Dot, text, span};
  case '|':
    return token::Token{token::TokenKind::Pipe, text, span};
  case '&':
    return token::Token{token::TokenKind::And, text, span};
  case '?':
    return token::Token{token::TokenKind::Question, text, span};
  default:
    return token::Token{token::TokenKind::Unsupported, text, span};
  };
}
