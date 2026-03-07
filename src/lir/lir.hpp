#ifndef SRC_LIR_LIR_HPP
#define SRC_LIR_LIR_HPP

#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lir {

struct BasicBlock;
struct Function;

struct Register {
  enum Kind : uint8_t { Virtual, Physical };
  Kind kind;
  unsigned id;

  static constexpr Register vreg(unsigned id) { return {Virtual, id}; }
  static constexpr Register preg(unsigned id) { return {Physical, id}; }

  [[nodiscard]] bool is_virtual() const { return kind == Virtual; }
  [[nodiscard]] bool is_physical() const { return kind == Physical; }

  bool operator==(const Register &o) const {
    return kind == o.kind && id == o.id;
  }
  bool operator!=(const Register &o) const { return !(*this == o); }

  [[nodiscard]] std::string to_string() const;
};

struct Operand {
  enum Kind : uint8_t { Reg, Imm, StackSlot, Block };
  Kind kind;

  union {
    Register reg;
    int64_t imm;
    int slot;
    BasicBlock *block;
  };

  [[nodiscard]] bool is_reg() const { return kind == Reg; }
  [[nodiscard]] bool is_imm() const { return kind == Imm; }
  [[nodiscard]] bool is_slot() const { return kind == StackSlot; }
  [[nodiscard]] bool is_block() const { return kind == Block; }

  [[nodiscard]] Register get_reg() const {
    assert(is_reg());
    return reg;
  }
  [[nodiscard]] int64_t get_imm() const {
    assert(is_imm());
    return imm;
  }
  [[nodiscard]] int get_slot() const {
    assert(is_slot());
    return slot;
  }
  [[nodiscard]] BasicBlock *get_block() const {
    assert(is_block());
    return block;
  }

  static Operand from_reg(Register r) {
    Operand o{};
    o.kind = Reg;
    o.reg = r;
    return o;
  }

  static Operand from_imm(int64_t v) {
    Operand o{};
    o.kind = Imm;
    o.imm = v;
    return o;
  }

  static Operand from_slot(int s) {
    Operand o{};
    o.kind = StackSlot;
    o.slot = s;
    return o;
  }

  static Operand from_block(BasicBlock *b) {
    Operand o{};
    o.kind = Block;
    o.block = b;
    return o;
  }

  [[nodiscard]] std::string to_string() const;
};

enum class Opcode : uint8_t {
  // Data movement
  Mov,  // dst = src
  Copy, // pseudo: dst = src (for SSA destruction, eliminated after regalloc)

  // Arithmetic
  Add,  // dst = lhs + rhs
  Sub,  // dst = lhs - rhs
  Mul,  // dst = lhs * rhs
  SDiv, // dst = lhs / rhs (signed)
  Neg,  // dst = -src (unary)

  // Bitwise
  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,

  // Compare + conditional
  Cmp,  // sets flags, no register output
  CSet, // dst = 1 if condition, 0 otherwise (reads flags)

  // Memory
  Load,  // dst = mem[addr]
  Store, // mem[addr] = src

  // Control flow
  Jump,     // unconditional branch to block
  CondJump, // conditional branch based on flags
  Call,     // dst = call func(args...)
  Ret,      // return [value]
};

[[nodiscard]] std::string_view opcode_name(Opcode op);

enum class CmpPredicate : uint8_t {
  EQ,
  NE,
  SLT,
  SLE,
  SGT,
  SGE,
  ULT,
  ULE,
  UGT,
  UGE
};

[[nodiscard]] std::string_view predicate_name(CmpPredicate pred);

// Operand layout by opcode:
//   Mov/Copy:  [dst, src]
//   Add/Sub/Mul/SDiv/And/Or/Xor/Shl/LShr/AShr:
//              [dst, lhs, rhs]
//   Neg:       [dst, src]
//   Cmp:       [lhs, rhs]         (predicate in metadata, sets flags)
//   CSet:      [dst]              (predicate in metadata, reads flags)
//   Load:      [dst, addr]
//   Store:     [addr, src]
//   Jump:      [block]
//   CondJump:  [true_block, false_block]  (reads flags, predicate in metadata)
//   Call:      [dst, arg0, arg1, ...]     (callee name in metadata)
//   Ret:       [value] or []              (void return)
//

struct Instruction {
  Opcode opcode;
  std::vector<Operand> operands;
  unsigned num_defs = 0;

  std::optional<CmpPredicate> predicate; // for Cmp, CSet, CondJump
  std::string callee;                    // for Call
  bool defs_flags = false;               // Cmp
  bool reads_flags = false;              // CSet, CondJump

  [[nodiscard]] Operand &def(unsigned i = 0) {
    assert(i < num_defs);
    return operands[i];
  }
  [[nodiscard]] const Operand &def(unsigned i = 0) const {
    assert(i < num_defs);
    return operands[i];
  }
  [[nodiscard]] Operand &use(unsigned i) { return operands[num_defs + i]; }
  [[nodiscard]] const Operand &use(unsigned i) const {
    return operands[num_defs + i];
  }

  [[nodiscard]] unsigned use_count() const {
    return static_cast<unsigned>(operands.size()) - num_defs;
  }

  [[nodiscard]] auto defs() { return std::span(operands.begin(), num_defs); }
  [[nodiscard]] auto defs() const {
    return std::span(operands.begin(), num_defs);
  }
  [[nodiscard]] auto uses() {
    return std::span(operands.begin() + num_defs, operands.end());
  }
  [[nodiscard]] auto uses() const {
    return std::span(operands.begin() + num_defs, operands.end());
  }

  [[nodiscard]] bool is_terminator() const {
    return opcode == Opcode::Jump || opcode == Opcode::CondJump ||
           opcode == Opcode::Ret;
  }

  [[nodiscard]] bool is_branch() const {
    return opcode == Opcode::Jump || opcode == Opcode::CondJump;
  }

  [[nodiscard]] bool is_call() const { return opcode == Opcode::Call; }

  [[nodiscard]] bool is_copy() const {
    return opcode == Opcode::Copy || opcode == Opcode::Mov;
  }

  [[nodiscard]] bool has_side_effects() const {
    return opcode == Opcode::Call || opcode == Opcode::Store;
  }

  [[nodiscard]] std::string to_string() const;
};

struct BasicBlock {
  std::string label;
  unsigned id;
  std::vector<Instruction *> instructions;
  Function *parent = nullptr;
  std::vector<BasicBlock *> predecessors;
  std::vector<BasicBlock *> successors;

  void add_successor(BasicBlock *succ) {
    successors.push_back(succ);
    succ->predecessors.push_back(this);
  }

  void emit(Instruction *instr) { instructions.push_back(instr); }

  [[nodiscard]] bool empty() const { return instructions.empty(); }

  [[nodiscard]] Instruction *terminator() const {
    if (instructions.empty())
      return nullptr;
    auto *last = instructions.back();
    return last->is_terminator() ? last : nullptr;
  }

  [[nodiscard]] std::string to_string() const;
};

struct Function {
  std::string name;
  std::vector<BasicBlock *> blocks;
  unsigned next_vreg_id = 0;
  unsigned next_block_id = 0;
  std::vector<Register> param_regs;
  bool has_return_value = true;
  bool has_calls = false;

  [[nodiscard]] BasicBlock *entry_block() const {
    assert(!blocks.empty());
    return blocks.front();
  }

  [[nodiscard]] std::string to_string() const;
};

struct Module {
  std::vector<Function *> functions;

  Function *add_function(std::string name);
  [[nodiscard]] Function *get_function(std::string_view name);

  [[nodiscard]] std::string to_string() const;
};

} // namespace lir

#endif // SRC_LIR_LIR_HPP
