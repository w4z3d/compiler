#ifndef CODE_GEN_TARGET_X86_H
#define CODE_GEN_TARGET_X86_H
#include "../../../mir/mir.hpp"
#include "../target.hpp"

class X86_64Target : public Target {
public:
  [[nodiscard]] const std::vector<mir::PhysicalRegister> get_gprs() const override {
    return {{"eax", 32},  {"ebx", 32},  {"ecx", 32},  {"edx", 32},
            {"esi", 32},  {"edi", 32},  {"r8d", 32},  {"r9d", 32},
            {"r10d", 32}, {"r11d", 32}, {"r12d", 32}, {"r13d", 32},
            {"r14d", 32}, {"r15d", 32}};
  }
  [[nodiscard]] size_t get_word_size() const override { return 16; }
  [[nodiscard]] size_t get_pointer_size() const override { return 64; }
  [[maybe_unused]] [[nodiscard]] Endianness get_endianness() const override {
    return Endianness::BIG;
  }
  [[nodiscard]] mir::PhysicalRegister get_instruction_pointer() const override {
    return {"rip", 64};
  }
  [[nodiscard]] mir::PhysicalRegister get_stack_pointer() const override {
    return {"rsp", 64};
  }
  [[nodiscard]] mir::PhysicalRegister get_base_pointer() const override {
    return {"rbp", 64};
  }
};

#endif // !CODE_GEN_TARGET_X86_H
