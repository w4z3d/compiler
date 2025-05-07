#ifndef TOKEN_H
#define TOKEN_H

#include <cmath>
#include <format>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace token {

enum class TokenKind {
  Eof,
  StringLiteral,
  Identifier,
  NumberLiteral,
    CharLiteral,
  At,
  BackSlash,

  // Separator
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Semi,

  // Unary Op
  Tilde,
  Bang,

  // Un Bi Ops
  Asterisk,
  Minus,

  // Binary Op
  Dot,
  Arrow,
  LAngleAngle,
  RAngleAngle,
  Slash,
  Plus,
  LAngleBracket,
  LessEqual,
  EqualEqual,
  BangEqual,
  GreaterEqual,
  RAngleBracket,
  And,
  AndAnd,
  Pipe,
  PipePipe,
  Caret, // ^
  Colon,
  Question,
  Percent,

  // Post Op
  PlusPlus,
  MinusMinus,

  // Assignment Ops
  PlusEquals,
  MinusEquals,
  AsteriskEquals,
  SlashEquals,
  PercentEquals,
  LAngleAngleEquals,
  RAngleAngleEquals,
  AndEquals,
  CaretEquals,
  Equals,
  PipeEquals,


  // Keywords
  Return,
  If,
  Else,
  While,
  For,
  Assert,
  Error,
  Struct,
  Typedef,
  Alloc,
  AllocArray,
  True,
  False,
  Null,
  Int,
  Bool,
  String,
  Char,
  Void,


  Unsupported = -1
};

const std::unordered_map<std::string_view, TokenKind> keyword_table = {
    {"return", TokenKind::Return},
    {"if", TokenKind::If},
    {"else", TokenKind::Else},
    {"while", TokenKind::While},
    {"for", TokenKind::For},
    {"assert", TokenKind::Assert},
    {"error", TokenKind::Error},
    {"struct", TokenKind::Struct},
    {"typedef", TokenKind::Typedef},
    {"alloc", TokenKind::Alloc},
    {"alloc_array", TokenKind::AllocArray},
    {"true", TokenKind::True},
    {"false", TokenKind::False},
    {"NULL", TokenKind::Null},
    {"int", TokenKind::Int},
    {"bool", TokenKind::Bool},
    {"string", TokenKind::String},
    {"char", TokenKind::Char},
    {"void", TokenKind::Void}
};

const std::unordered_set<token::TokenKind> binary_ops = {
    token::TokenKind::Plus,
    token::TokenKind::Minus,
    token::TokenKind::Dot,
    token::TokenKind::Arrow,
    token::TokenKind::Asterisk,
    token::TokenKind::Slash,
    token::TokenKind::Percent,
    token::TokenKind::LAngleAngle,
    token::TokenKind::RAngleAngle,
    token::TokenKind::LAngleBracket,
    token::TokenKind::RAngleBracket,
    token::TokenKind::LessEqual,
    token::TokenKind::GreaterEqual,
    token::TokenKind::EqualEqual,
    token::TokenKind::BangEqual,
    token::TokenKind::And,
    token::TokenKind::PipePipe,
    token::TokenKind::AndAnd,
    token::TokenKind::Pipe,
    token::TokenKind::Caret
};

const std::unordered_set<token::TokenKind> unary_ops = {
    token::TokenKind::Bang,
    token::TokenKind::Minus,
    token::TokenKind::Tilde,
    token::TokenKind::Asterisk
};

const std::unordered_set<token::TokenKind> assignment_ops = {
    token::TokenKind::PlusEquals,
    token::TokenKind::MinusEquals,
    token::TokenKind::AsteriskEquals,
    token::TokenKind::SlashEquals,
    token::TokenKind::PercentEquals,
    token::TokenKind::LAngleAngleEquals,
    token::TokenKind::RAngleAngleEquals,
    token::TokenKind::AndEquals,
    token::TokenKind::CaretEquals,
    token::TokenKind::Equals,
    token::TokenKind::PipeEquals,
};

const std::unordered_set<token::TokenKind> post_ops = {
    token::TokenKind::PlusPlus,
    token::TokenKind::MinusMinus
};

struct Span {
  std::string_view source_file;
  std::tuple<int, int> start;
  std::tuple<int, int> end;
};

inline std::string token_kind_to_string(TokenKind kind) {
  switch (kind) {
    case TokenKind::Percent:
      return "Percent";
    case TokenKind::MinusMinus:
      return "MinusMinus";
    case TokenKind::PlusPlus:
      return "PlusPlus";
    case TokenKind::AndAnd:
      return "AndAnd";
    case TokenKind::PipePipe:
      return "PipePipe";
    case TokenKind::Caret:
      return "Caret";
    case TokenKind::Tilde:
      return "Tilde";
    case TokenKind::Arrow:
      return "Arrow";
    case TokenKind::LAngleAngle:
      return "LAngleAngle";
    case TokenKind::RAngleAngle:
      return "RAngleAngle";
    case TokenKind::LAngleBracket:
      return "LAngleBracket";
    case TokenKind::EqualEqual:
      return "EqualEqual";
    case TokenKind::LessEqual:
      return "LessEqual";
    case TokenKind::RAngleBracket:
      return "RAngleBracket";
    case TokenKind::GreaterEqual:
      return "GreaterEqual";
    case TokenKind::BangEqual:
      return "BangEqual";
    case TokenKind::StringLiteral:
      return "StringLiteral";
    case TokenKind::Eof:
      return "Eof";
    case TokenKind::Identifier:
      return "Identifier";
    case TokenKind::NumberLiteral:
      return "NumberLiteral";
    case TokenKind::CharLiteral:
      return "CharLiteral";
    case TokenKind::Plus:
      return "Plus";
    case TokenKind::Minus:
      return "Minus";
    case TokenKind::LParen:
      return "LParen";
    case TokenKind::RParen:
      return "RParen";
    case TokenKind::LBrace:
      return "LBrace";
    case TokenKind::RBrace:
      return "RBrace";
    case TokenKind::LBracket:
      return "LBracket";
    case TokenKind::RBracket:
      return "RBracket";
    case TokenKind::Slash:
      return "Slash";
    case TokenKind::Asterisk:
      return "Asterisk";
    case TokenKind::Comma:
      return "Comma";
    case TokenKind::Semi:
      return "Semi";
    case TokenKind::At:
      return "At";
    case TokenKind::BackSlash:
      return "BackSlash";
    case TokenKind::Equals:
      return "Equals";
    case TokenKind::Bang:
      return "Bang";
    case TokenKind::Colon:
      return "Colon";
    case TokenKind::Dot:
      return "Dot";
    case TokenKind::Pipe:
      return "Pipe";
    case TokenKind::And:
      return "And";
    case TokenKind::Question:
      return "Question";
    case TokenKind::Return:
      return "Return";
    case TokenKind::If:
      return "If";
    case TokenKind::Else:
      return "Else";
    case TokenKind::While:
      return "While";
    case TokenKind::For:
      return "For";
    case TokenKind::Assert:
      return "Assert";
    case TokenKind::Error:
      return "Error";
    case TokenKind::Struct:
      return "Struct";
    case TokenKind::Typedef:
      return "Typedef";
    case TokenKind::Alloc:
      return "Alloc";
    case TokenKind::AllocArray:
      return "AllocArray";
    case TokenKind::True:
      return "True";
    case TokenKind::False:
      return "False";
    case TokenKind::Null:
      return "Null";
    case TokenKind::PlusEquals:
      return "PlusEquals";
    case TokenKind::MinusEquals:
      return "MinusEquals";
    case TokenKind::AsteriskEquals:
      return "AsteriskEquals";
    case TokenKind::SlashEquals:
      return "SlashEquals";
    case TokenKind::PercentEquals:
      return "PercentEquals";
    case TokenKind::LAngleAngleEquals:
      return "LAngleAngleEquals";
    case TokenKind::RAngleAngleEquals:
      return "RAngleAngleEquals";
    case TokenKind::AndEquals:
      return "AndEquals";
    case TokenKind::CaretEquals:
      return "CaretEquals";
    case TokenKind::PipeEquals:
      return "PipeEquals";
    case TokenKind::Int:
      return "Int";
    case TokenKind::Bool:
      return "Bool";
    case TokenKind::String:
      return "String";
    case TokenKind::Char:
      return "Char";
    case TokenKind::Void:
      return "Void";
    default:
      return "Unsupported";
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
