#ifndef TOKEN_H
#define TOKEN_H

#include <cmath>
#include <format>
#include <memory>
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

class Token {
public:
  TokenKind kind;
  std::string_view text;
  Span span;

  bool invalid = false;

  explicit operator std::string() const {
    std::string kind_string;
    switch (kind) {
    case TokenKind::String:
      kind_string = "String";
      break;
    case TokenKind::Eof:
      kind_string = "Eof";
      break;
    case TokenKind::Identifier:
      kind_string = "Identifier";
      break;
    case TokenKind::Number:
      kind_string = "Number";
      break;
    case TokenKind::Char:
      kind_string = "Char";
      break;
    case TokenKind::Plus:
      kind_string = "Plus";
      break;
    case TokenKind::Minus:
      kind_string = "Minus";
      break;
    case TokenKind::LParen:
      kind_string = "LParen";
      break;
    case TokenKind::RParen:
      kind_string = "RParen";
      break;
    case TokenKind::LBrace:
      kind_string = "LBrace";
      break;
    case TokenKind::RBrace:
      kind_string = "RBrace";
      break;
    case TokenKind::LBracket:
      kind_string = "LBracket";
      break;
    case TokenKind::RBracket:
      kind_string = "RBracket";
      break;
    case TokenKind::Slash:
      kind_string = "Slash";
      break;
    case TokenKind::Asterisk:
      kind_string = "Asterisk";
      break;
    case TokenKind::Hash:
      kind_string = "Hash";
      break;
    case TokenKind::LAngleBracket:
      kind_string = "LAngleBracket";
      break;
    case TokenKind::RAngleBracket:
      kind_string = "RAngleBracket";
      break;
    case TokenKind::Comma:
      kind_string = "Comma";
      break;
    case TokenKind::Semi:
      kind_string = "Semi";
      break;
    case TokenKind::At:
      kind_string = "At";
      break;
    case TokenKind::BackSlash:
      kind_string = "BackSlash";
      break;
    case TokenKind::Equals:
      kind_string = "Equals";
      break;
    case TokenKind::Exclamation:
      kind_string = "Exclamation";
      break;
    case TokenKind::Colon:
      kind_string = "Colon";
      break;
    case TokenKind::Dot:
      kind_string = "Dot";
      break;
    case TokenKind::Pipe:
      kind_string = "Pipe";
      break;
    case TokenKind::And:
      kind_string = "And";
      break;
    case TokenKind::Question:
      kind_string = "Question";
      break;
    case TokenKind::Unsupported:
      kind_string = "Unsupported";
      break;
    }
    return std::format("{:12} \'{:12}\' <ll. {}:{} - {}:{}> ", kind_string,
                       text, std::get<0>(span.start), std::get<1>(span.start),
                       std::get<0>(span.end), std::get<1>(span.end));
  }
};

} // namespace token

#endif // TOKEN_H
