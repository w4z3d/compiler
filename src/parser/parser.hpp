#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H
#include "../alloc/arena.hpp"
#include "../defs/ast.hpp"
#include "../lexer/lexer.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <exception>
#include <optional>
#include <utility>
#include <vector>

class ParseError : public std::exception {
  std::string message;

public:
  explicit ParseError(std::string message) : message(std::move(message)) {}
  [[nodiscard]] const char *what() const noexcept override {
    return message.c_str();
  }
};

class Parser {
private:
  Lexer lexer;
  arena::Arena arena;

  std::vector<token::Token> token_buffer;

  token::Token peek(std::size_t n = 0) {
    while (token_buffer.size() <= n) {
      const auto next_token{lexer.next_token()};
      spdlog::log(spdlog::level::debug, "Peeking {}", std::string{next_token});
      token_buffer.push_back(next_token);
    }
    return token_buffer[n];
  }

  bool check_sequence(const std::vector<token::TokenKind> &sequence) {
    for (std::size_t i = 0; i < sequence.size(); i++) {
      if (peek(i).kind != sequence[i]) {
        return false;
      }
    }

    return true;
  }

  bool is_next(const token::TokenKind token) { return peek().kind == token; }

  token::Token next_token() {
    if (token_buffer.empty()) {
      const auto token{lexer.next_token()};
      return token;
    } else {
      const auto result{token_buffer.front()};
      spdlog::log(spdlog::level::debug, "Lexing {}", std::string{result});
      token_buffer.erase(token_buffer.begin());
      return result;
    }
  }

  std::optional<token::Token> expect(token::TokenKind expected) {
    const auto token{peek()};
    if (token.kind == expected) {
      next_token();
      return token;
    }
    throw ParseError{std::format(
        "Unexpected {} token on line {}:{} \'{}\' expected {}",
        token_kind_to_string(token.kind), std::get<0>(token.span.start),
        std::get<1>(token.span.start), token.text,
        token::token_kind_to_string(expected))};
    return std::nullopt;
  }

  bool match(token::TokenKind expected) noexcept {
    const auto token{peek()};
    if (token.kind == expected) {
      next_token();
      return true;
    }
    return false;
  }

  void synchronize() {
    next_token();

    while (peek().kind != token::TokenKind::Eof) {
      if (peek().kind == token::TokenKind::Semi) {
        next_token();
        return;
      }
      next_token();
    }
  }

  FunctionDeclaration *parse_function_declaration();
  ParameterDeclaration *parse_parameter_declaration();

  CompoundStmt *parse_compound_statement();
  ReturnStmt *parse_return_statement();
  Statement *parse_statement();

  Expression *parse_expression();
  NumericExpression *parse_integer_literal();

public:
  TranslationUnit *parse_translation_unit();

  bool is_eof() { return peek().kind == token::TokenKind::Eof; }
  explicit Parser(Lexer lexer) : lexer(lexer), arena(arena::Arena{}) {}
};

#endif
