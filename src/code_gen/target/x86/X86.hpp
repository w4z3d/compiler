#ifndef CODE_GEN_TARGET_X86_H
#define CODE_GEN_TARGET_X86_H
#include "../../../mir/mir.hpp"
inline static std::vector<mir::PhysicalRegister>
get_general_purpose_register() {
  return {{"eax", 32},  {"ebx", 32},  {"ecx", 32},  {"edx", 32},
          {"esi", 32},  {"edi", 32},  {"ebp", 32},  {"esp", 32},
          {"r8d", 32},  {"r9d", 32},  {"r10d", 32}, {"r11d", 32},
          {"r12d", 32}, {"r13d", 32}, {"r14d", 32}, {"r15d", 32}};
}

#endif // !CODE_GEN_TARGET_X86_H
