#ifndef TOKEN_H
#define TOKEN_H

#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <tuple>
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
  Slash,
  Asterisk,
  Hash,
  LAngleBracket,
  RAngleBracket,
  Comma,
  Semi,
  At,
  BackSlash,
  Equals,
  Exclamation,
  Colon,
  Dot,
  Pipe,
  And,
  Question,

  Unsupported = -1
};

struct Span {
  std::string_view source_file;
  std::tuple<int, int> start;
  std::tuple<int, int> end;
};

inline std::string token_kind_to_string(TokenKind kind) {
  switch (kind) {
  case TokenKind::String:
    return "String";
    break;
  case TokenKind::Eof:
    return "Eof";
    break;
  case TokenKind::Identifier:
    return "Identifier";
    break;
  case TokenKind::Number:
    return "Number";
    break;
  case TokenKind::Char:
    return "Char";
    break;
  case TokenKind::Plus:
    return "Plus";
    break;
  case TokenKind::Minus:
    return "Minus";
    break;
  case TokenKind::LParen:
    return "LParen";
    break;
  case TokenKind::RParen:
    return "RParen";
    break;
  case TokenKind::LBrace:
    return "LBrace";
    break;
  case TokenKind::RBrace:
    return "RBrace";
    break;
  case TokenKind::LBracket:
    return "LBracket";
    break;
  case TokenKind::RBracket:
    return "RBracket";
    break;
  case TokenKind::Slash:
    return "Slash";
    break;
  case TokenKind::Asterisk:
    return "Asterisk";
    break;
  case TokenKind::Hash:
    return "Hash";
    break;
  case TokenKind::LAngleBracket:
    return "LAngleBracket";
    break;
  case TokenKind::RAngleBracket:
    return "RAngleBracket";
    break;
  case TokenKind::Comma:
    return "Comma";
    break;
  case TokenKind::Semi:
    return "Semi";
    break;
  case TokenKind::At:
    return "At";
    break;
  case TokenKind::BackSlash:
    return "BackSlash";
    break;
  case TokenKind::Equals:
    return "Equals";
    break;
  case TokenKind::Exclamation:
    return "Exclamation";
    break;
  case TokenKind::Colon:
    return "Colon";
    break;
  case TokenKind::Dot:
    return "Dot";
    break;
  case TokenKind::Pipe:
    return "Pipe";
    break;
  case TokenKind::And:
    return "And";
    break;
  case TokenKind::Question:
    return "Question";
    break;
  case TokenKind::Unsupported:
    return "Unsupported";
    break;
  }
}

class Token {
public:
  TokenKind kind;
  std::string_view text;
  Span span;

  bool invalid = false;

  explicit operator std::string() const {
    return std::format("{} \'{}\' <ll. {}:{} - {}:{}> ",
                       token_kind_to_string(kind), text,
                       std::get<0>(span.start), std::get<1>(span.start),
                       std::get<0>(span.end), std::get<1>(span.end));
  }
};

} // namespace token

#endif // TOKEN_H
