#include "lexer.hpp"
#include <cctype>
#include <string_view>
#include <tuple>
#include <iostream>

Lexer::Lexer(std::string_view file_name, std::string_view source)
    : file_name(file_name), source(source) {}

char Lexer::peek(int lookahead) const {
  return index + lookahead < source.size() ? source[index + lookahead] : '\0';
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

void Lexer::skip_oneline_comment() {
  while (peek() != '\n' && peek() != '\0')
    get();
}

// TODO: fix for nested multiline comments...
void Lexer::skip_multiline_comment() {
  while (true) {
    if (peek() == '\0') {
      break;
    }
    if (peek() == '*') {
      get();
      if (peek() == '/') {
        get();
        break;
      }
    } else {
      get();
    }
  }
}

token::Token Lexer::next_token() {
  skip_whitespace();

  if (eof()) {
    return {token::TokenKind::Eof, "EOF",
            token::Span{file_name, std::make_tuple(line, column),
                        std::make_tuple(line, column)}};
  }

  char c = peek();

  // remove comments
  if (c == '/') {
    if (peek(1) == '/') {
      skip_oneline_comment();
      // there could be whitespace again after comment...
      return next_token();
    } else if (peek(1) == '*') {
      skip_multiline_comment();
      // there could be whitespace again after comment...
      return next_token();
    }
  }

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
  if (token::keyword_table.contains(ident)) {
    return token::Token{token::keyword_table.at(ident), ident, span};
  }
  return token::Token{token::TokenKind::Identifier, ident, span};
}

token::Token Lexer::lex_number() {
  size_t start_index = index;
  const auto start = std::make_tuple(line, column);

  if (peek(0) == '0' && (peek(1) == 'x' || peek(1) == 'X') &&
      std::isxdigit(peek(2))) {
    get();
    get();
    while (std::isxdigit(peek()))
      get();
    std::string_view text = source.substr(start_index, index - start_index);
    const auto end = std::make_tuple(line, column);
    token::Span span{file_name, start, end};
    return token::Token{token::TokenKind::NumberLiteralHex, text, span};
  }

  while (std::isdigit(peek()))
    get();

  std::string_view text = source.substr(start_index, index - start_index);
  const auto end = std::make_tuple(line, column);
  token::Span span{file_name, start, end};
  return token::Token{token::TokenKind::NumberLiteralDec, text, span};
}

// TODO: add escape literals
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
  return token::Token{token::TokenKind::StringLiteral, text, span};
}

token::Token Lexer::lex_char_literal() {
  size_t start_index = index;
  const auto start = std::make_tuple(line, column);
  bool is_invalid = false;

  get(); // consume opening '

  if (eof() || peek() == '\n' || peek() == '\'') {
    is_invalid = true;
  } else if (peek() == '\\') {
    get(); // consume '\'
    char esc = peek();
    switch (esc) {
    case 'n':
    case 't':
    case 'r':
    case '\\':
    case '\'':
    case '\"':
    case '0':
      get(); // consume escape char
      break;
    default:
      get(); // consume invalid escape anyway
      is_invalid = true;
      break;
    }
  } else {
    get(); // consume regular char
  }

  if (peek() != '\'') {
    is_invalid = true;
  } else {
    get(); // consume closing '
  }

  std::string_view text = source.substr(start_index, index - start_index);
  const auto end = std::make_tuple(line, column);
  token::Span span{file_name, start, end};
  return token::Token{token::TokenKind::CharLiteral, text, span, is_invalid};
}

token::Token Lexer::lex_operator_or_punctuation() {
  auto i = index;
  const auto start{std::make_tuple(line, column)};
  token::TokenKind tokenKind;

  char c = get();
  switch (c) {
  case '+':
    if (peek() == '+') {
      get();
      tokenKind = token::TokenKind::PlusPlus;
    } else if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::PlusEquals;
    } else {
      tokenKind = token::TokenKind::Plus;
    }
    break;
  case '-':
    if (peek() == '-') {
      get();
      tokenKind = token::TokenKind::MinusMinus;
    } else if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::MinusEquals;
    } else if (peek() == '>') {
      get();
      tokenKind = token::TokenKind::Arrow;
    } else {
      tokenKind = token::TokenKind::Minus;
    }
    break;
  case '^':
    if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::CaretEquals;
    } else {
      tokenKind = token::TokenKind::Caret;
    }
    break;
  case '~':
    tokenKind = token::TokenKind::Tilde;
    break;
  case '%':
    if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::PercentEquals;
    } else {
      tokenKind = token::TokenKind::Percent;
    }
    break;
  case '(':
    tokenKind = token::TokenKind::LParen;
    break;
  case ')':
    tokenKind = token::TokenKind::RParen;
    break;
  case '{':
    tokenKind = token::TokenKind::LBrace;
    break;
  case '}':
    tokenKind = token::TokenKind::RBrace;
    break;
  case '[':
    tokenKind = token::TokenKind::LBracket;
    break;
  case ']':
    tokenKind = token::TokenKind::RBracket;
    break;
  case '/':
    if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::SlashEquals;
    } else {
      tokenKind = token::TokenKind::Slash;
    }
    break;
  case '*':
    if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::AsteriskEquals;
    } else {
      tokenKind = token::TokenKind::Asterisk;
    }
    break;
  case '<':
    if (peek() == '<') {
      get();
      if (peek() == '=') {
        get();
        tokenKind = token::TokenKind::LAngleAngleEquals;
      } else {
        tokenKind = token::TokenKind::LAngleAngle;
      }
    } else if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::LessEqual;
    } else {
      tokenKind = token::TokenKind::LAngleBracket;
    }
    break;
  case '>':
    if (peek() == '>') {
      get();
      if (peek() == '=') {
        get();
        tokenKind = token::TokenKind::RAngleAngleEquals;
      } else {
        tokenKind = token::TokenKind::RAngleAngle;
      }
    } else if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::GreaterEqual;
    } else {
      tokenKind = token::TokenKind::RAngleBracket;
    }
    break;
  case ',':
    tokenKind = token::TokenKind::Comma;
    break;
  case ';':
    tokenKind = token::TokenKind::Semi;
    break;
  case '@':
    tokenKind = token::TokenKind::At;
    break;
  case '\\':
    tokenKind = token::TokenKind::BackSlash;
    break;
  case '=':
    if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::EqualEqual;
    } else {
      tokenKind = token::TokenKind::Equals;
    }
    break;
  case '!':
    if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::BangEqual;
    } else {
      tokenKind = token::TokenKind::Bang;
    }
    break;
  case ':':
    tokenKind = token::TokenKind::Colon;
    break;
  case '.':
    tokenKind = token::TokenKind::Dot;
    break;
  case '|':
    if (peek() == '|') {
      get();
      tokenKind = token::TokenKind::PipePipe;
    } else if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::PipeEquals;
    } else {
      tokenKind = token::TokenKind::Pipe;
    }
    break;
  case '&':
    if (peek() == '&') {
      get();
      tokenKind = token::TokenKind::AndAnd;
    } else if (peek() == '=') {
      get();
      tokenKind = token::TokenKind::AndEquals;
    } else {
      tokenKind = token::TokenKind::And;
    }
    break;
  case '?':
    tokenKind = token::TokenKind::Question;
    break;
  default:
    tokenKind = token::TokenKind::Unsupported;
    break;
  };
  std::string_view text = source.substr(i, index - i);
  const auto end{std::make_tuple(line, column)};
  const token::Span span{file_name, start, end};
  return token::Token{tokenKind, text, span};
}
