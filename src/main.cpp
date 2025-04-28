#include "alloc/arena.hpp"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"
#include <cstdint>

struct Test {
  std::uint8_t alignme;
  std::size_t test_alignment;
};

int main(int argc, char *argv[]) {

  arena::Arena arena{};

  const auto test = arena.create<Test>(10, 200);
  const auto test2 = arena.create<Test>(20, 300);
  const auto test3 = arena.create<Test>(30, 400);

  spdlog::log(spdlog::level::info, "Struct: {} {}, {}", test->test_alignment,
              test2->alignme, test3->test_alignment);
  return 0;
}
