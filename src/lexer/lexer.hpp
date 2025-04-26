#ifndef LEXER_LEXER_H
#define LEXER_LEXER_H
#include <cstdint>
#include <cstdio>
#include <fmt/base.h>
#include <lexy/action/parse.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>

struct Color {
  std::uint8_t r, g, b;
};

namespace grammar {
namespace dsl = lexy::dsl;
struct channel {
  static constexpr auto rule =
      dsl::integer<std::uint8_t>(dsl::n_digits<2, dsl::hex>);
  static constexpr auto value = lexy::forward<std::uint8_t>;
};

struct color {
  static constexpr auto rule = dsl::hash_sign + dsl::times<3>(dsl::p<channel>);
  static constexpr auto value = lexy::construct<Color>;
};

void parse();
} // namespace grammar

#endif // !LEXER_LEXER_H
