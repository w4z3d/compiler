#include "lexer.hpp"
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string_view>

void grammar::parse() {
  constexpr std::string_view s{"#FFFFFF"};
  constexpr auto input = lexy::string_input(s);
  auto result = lexy::parse<grammar::color>(input, lexy_ext::report_error);
  if (result.has_value()) {
    auto color = result.value();
    spdlog::log(spdlog::level::info, "{} {} {}", color.r, color.g, color.b);
  }
}
