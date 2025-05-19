#ifndef COMPILER_TARGET_BUILDER_H
#define COMPILER_TARGET_BUILDER_H

#include "target.hpp"
#include "x86/X86.hpp"

template <typename T>
static T create_compiler_target(CompilerTarget t) {
  switch (t) {
  case CompilerTarget::X86_64:
    return X86_64Target{};
  default:
    throw std::runtime_error("Succ succ succ");
  }
}

#endif // COMPILER_TARGET_BUILDER_H
