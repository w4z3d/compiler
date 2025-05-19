#ifndef COMPILER_TARGET_H
#define COMPILER_TARGET_H

#include "../../mir/mir.hpp"
#include <vector>

enum class CompilerTarget {
  X86_64
};

class Target {
public:
  enum class Endianness {
    BIG, SMALL
  };
  [[nodiscard]] virtual const std::vector<mir::PhysicalRegister> get_gprs() const = 0;
  // in bits
  [[nodiscard]] virtual size_t get_word_size() const = 0;
  // in bits
  [[nodiscard]] virtual size_t get_pointer_size() const = 0;
  [[nodiscard]] virtual mir::PhysicalRegister get_instruction_pointer() const = 0;
  [[nodiscard]] virtual mir::PhysicalRegister get_stack_pointer() const = 0;
  [[nodiscard]] virtual mir::PhysicalRegister get_base_pointer() const = 0;
  [[maybe_unused]] [[nodiscard]] virtual Endianness get_endianness() const = 0;
};

#endif // COMPILER_TARGET_H
