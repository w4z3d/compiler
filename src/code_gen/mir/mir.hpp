#ifndef MIR_MIR_H
#define MIR_MIR_H
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mir {

/**
 * @brief Class of register (used for later floating point calculations)
 */
enum class RegisterClass { GPR };

/**
 * @brief Virtual and Physical Register definition.
 * The actual registers should be filled in the target specific namespaces
 */
struct Register {
  std::size_t id;
  RegisterClass register_class;
  bool is_virtual;

  static Register vreg(std::size_t id, RegisterClass register_class) {
    return Register{id, true, register_class};
  }
  static Register preg(std::size_t id, RegisterClass register_class) {
    return Register{id, false, register_class};
  }

private:
  explicit Register(std::size_t id, bool is_virtual,
                    RegisterClass register_class)
      : id(id), is_virtual(is_virtual), register_class(register_class) {}
};

/**
 * @brief And immediate operand value
 */
struct Immediate {
  std::int32_t value;
};

/**
 * @brief The kind of operand
 */
enum class OperandKind { Register, Immediate, Stack, Memory };

/**
 * @brief An Operand can be either a virtual/physical register, and immediate
 * value, a stack value, or a memory access
 */
struct Operand {
  OperandKind _operand_kind;
  // Damn c++ keywords GRRRRR
  std::optional<Register> _register;
  std::optional<Immediate> _immediate;

  /**
   * Helper methods for creating operands
   */
  static Operand reg(Register reg) {
    return Operand{reg, std::nullopt, OperandKind::Register};
  }
  static Operand imm(Immediate imm) {
    return Operand(std::nullopt, imm, OperandKind::Immediate);
  }

private:
  explicit Operand(std::optional<Register> reg,
                   std::optional<Immediate> immediate, OperandKind kind)
      : _register(reg), _immediate(immediate), _operand_kind(kind) {}
};

/**
 * @brief Representing an instruction in the MIR cfg.
 * The opcodes for the MIR Instructions should be defined in the Targets
 * respectively, because the MIR is implemented as runtime target specific,
 * meaning the opcodes will be selected based on the target specified in the
 * params of execution
 */
struct Instruction {
  /**
   * @brief All of the operands that this instruction uses (important for
   * register allocation)
   */
  std::vector<Operand> uses;

  /**
   * @brief All of the operands that this instruction defines (important for
   * register allocation especially precoloring)
   */
  std::vector<Operand> defs;
};

struct BasicBlock {
  std::uint32_t id;
  std::vector<Instruction> instructions;
  std::vector<BasicBlock *> predecessors;
  std::vector<BasicBlock *> successors;
};

/**
 * @brief The MIR Function is a SSA conform Control Flow Graph oriented data
 * structure.
 * Being in SSA form we ensure the generation of chordal
 * intereference graphs.
 */
struct Function {
  std::string_view name;
  BasicBlock entry_block;
};

} // namespace mir
#endif // !MIR_MIR_H
