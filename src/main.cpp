#include "alloc/arena.hpp"
#include "defs/token.hpp"
#include "lexer/lexer.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <string>
#include <vector>

struct Test {
  std::uint8_t alignme;
  std::size_t test_alignment;
};

int main(int argc, char *argv[]) {

  arena::Arena arena{16000};
  Test *test = arena.make(Test{8, 100});

  spdlog::log(spdlog::level::info, "Struct: {} {}", test->test_alignment,
              test->alignme);
  return 0;
}
