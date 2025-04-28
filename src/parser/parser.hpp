#ifndef PARSER_PARSER_H
#define PARSER_PARSER_H
#include "../alloc/arena.hpp"
#include "../defs/ast.hpp"
#include "../lexer/lexer.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include <cstddef>
#include <iostream>
#include <optional>
#include <vector>

class Parser {
private:
  Lexer lexer;
  arena::Arena arena;
  TranslationUnit *unit;

  std::vector<token::Token> token_buffer;

  const token::Token peek(std::size_t n = 0) {
    while (token_buffer.size() <= n) {
      const auto next_token{lexer.next_token()};
      spdlog::log(spdlog::level::info, "Peeking {}", std::string{next_token});
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

  const token::Token next_token() {
    if (token_buffer.empty()) {
      return lexer.next_token();
    } else {
      const auto result{token_buffer.front()};
      token_buffer.erase(token_buffer.begin());
      return result;
    }
  }

  const std::optional<token::Token> expect(token::TokenKind expected) noexcept {
    const auto token{peek()};
    if (token.kind == expected) {
      next_token();
      return token;
    }
    spdlog::log(spdlog::level::err, "Unexpected token {}", std::string{token});
    return std::nullopt;
  }

  const bool match(token::TokenKind expected) noexcept {
    const auto token{peek()};
    if (token.kind == expected) {
      next_token();
      return true;
    }
    return false;
  }

  FunctionDeclaration *parse_function_declaration();

public:
  TranslationUnit *parse_translation_unit();

  const bool is_eof() { return peek().kind == token::TokenKind::Eof; }
  Parser(Lexer lexer) : lexer(lexer), arena(arena::Arena{}) {}
};

#endif
