#ifndef TOKEN_H
#define TOKEN_H

#include <string_view>
namespace token {

enum class TokenKind {
  String,
  Eof,
  Identifier,
  Number,
  Char,
  Plus,
  Minus,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,

  Unsupported = -1
};

struct Token {
  TokenKind kind;
  std::string_view text;
  int line;
  int column;

  bool invalid = false;
};

} // namespace token

#endif // TOKEN_H
