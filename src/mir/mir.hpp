#ifndef MIR_MIR_H
#define MIR_MIR_H

#include <cstddef>
#include <cstdint>
#include <format>
#include <list>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace mir {

class Register;

class StackSlot {
public:
  std::size_t offset;
  explicit StackSlot(size_t offset) : offset(offset) {}
};

class Immediate {
public:
  std::int32_t value;
  explicit Immediate(int32_t value) : value(value) {}
};

class MemoryAccess {
public:
  Register &base_register;
  std::size_t offset;
  MemoryAccess(Register &baseRegister, size_t offset)
      : base_register(baseRegister), offset(offset) {}
};

class Register {
private:
  std::string name;
  int bit_size;

public:
  Register(std::string name, int bit_size)
      : name(std::move(name)), bit_size(bit_size) {}
  [[nodiscard]] std::string get_name() const { return name; }
  bool operator==(Register other) const {
    return name == other.name;
  }
};

class VirtualRegister : public Register {
private:
  std::size_t numeral;

public:
  VirtualRegister(std::size_t numeral, int bit_size)
      : Register(std::format("vreg{}", numeral), bit_size), numeral(numeral) {}
  [[nodiscard]] std::size_t get_numeral() const { return numeral; }
};

class PhysicalRegister : public Register {
public:
  PhysicalRegister(std::string name, int bit_size)
      : Register(std::move(name), bit_size) {}
};

struct MachineOperand {
private:
  std::variant<VirtualRegister, PhysicalRegister, StackSlot, Immediate,
                     MemoryAccess>
      operand;

public:
  explicit MachineOperand(const VirtualRegister &op) : operand(op) {}
  explicit MachineOperand(const PhysicalRegister &op) : operand(op) {}
  explicit MachineOperand(const StackSlot &op) : operand(op) {}
  explicit MachineOperand(const Immediate &op) : operand(op) {}
  explicit MachineOperand(const MemoryAccess &op) : operand(op) {}
  [[nodiscard]] const std::variant<VirtualRegister, PhysicalRegister, StackSlot,
                                   Immediate, MemoryAccess> &
  get_op() const {
    return operand;
  }

  void replace_with_physical(PhysicalRegister r) {
    operand = r;
  }

  void replace_with_stack_slot(StackSlot s) {
    operand = s;
  }

  MachineOperand(const MachineOperand &ref) = default;
  explicit MachineOperand(
      const std::variant<VirtualRegister, PhysicalRegister, StackSlot,
                         Immediate, MemoryAccess> &operand)
      : operand(operand) {}
};

struct MachineInstruction {
public:
  enum class MachineOpcode {
    MOV_RR,
    MOV_RI,
    STORE_MEM_REG,
    LOAD_REG_MEM,

    ADD_RR, // inout:dest += in:src
    ADD_RI, // inout:dest += in:imm

    SUB_RR, // inout:dest -= in:src
    SUB_RI, // inout:dest -= in:imm

    DIV_RR, // out:dst = in:dst / in:src out:rem = in:dst % in:src
    DIV_RI, // out:dst = in:dst / in:imm out:rem = in:dst % in:src

    MOD_RR, // out:dst = in:dst / in:src out:rem = in:dst % in:src
    MOD_RI, // out:dst = in:dst / in:imm out:rem = in:dst % in:src

    MUL_RR, // inout:dest *= in:src
    MUL_RI, // inout:dest *= in:imm

    NEG_R, // out:dest = -in:dest

    RET // in:eax
  };

private:
  MachineOpcode opcode;
  std::vector<MachineOperand> ins;
  std::vector<MachineOperand> outs;
  std::vector<MachineOperand> implicit_defs;
  std::vector<MachineOperand> implicit_uses;

public:
  explicit MachineInstruction(MachineOpcode opcode,
                              std::vector<MachineOperand> ins = {},
                              std::vector<MachineOperand> outs = {},
                              std::vector<MachineOperand> implicit_def = {},
                              std::vector<MachineOperand> implicit_use = {})
      : opcode(opcode), ins(std::move(ins)), outs(std::move(outs)),
        implicit_uses(std::move(implicit_use)),
        implicit_defs(std::move(implicit_def)) {}

  void set_opcode(MachineOpcode opcode) {
    this->opcode = opcode;
  }

  void add_in(const MachineOperand &op) { ins.push_back(op); }
  [[nodiscard]] const std::vector<MachineOperand> &get_ins() const {
    return ins;
  }

  std::vector<MachineOperand> &get_ins_mut() {
    return ins;
  }

  void add_out(const MachineOperand &op) { outs.push_back(op); }
  [[nodiscard]] const std::vector<MachineOperand> &get_outs() const {
    return outs;
  }

  std::vector<MachineOperand> &get_outs_mut() {
    return outs;
  }

  void add_implicit_def(const MachineOperand &op) {
    implicit_defs.push_back(op);
  }
  [[nodiscard]] const std::vector<MachineOperand> &get_implicit_defs() const {
    return implicit_defs;
  }
  void add_implicit_use(const MachineOperand &op) {
    implicit_uses.push_back(op);
  }
  [[nodiscard]] const std::vector<MachineOperand> &get_implicit_uses() const {
    return implicit_uses;
  }
  [[nodiscard]] MachineOpcode get_opcode() const { return opcode; }
};

struct MachineBasicBlock {
  std::list<MachineInstruction *> instructions{};
  std::vector<MachineBasicBlock *> successors{};
  std::vector<MachineBasicBlock *> predecessors{};

  std::size_t id;
  inline static std::size_t id_counter = 0;

public:
  MachineBasicBlock() : id(id_counter++) {}

  void add_instruction(MachineInstruction *instruction) {
    instructions.push_back(instruction);
  }
  std::list<MachineInstruction *> &get_instructions() { return instructions; }

  void add_successor(MachineBasicBlock *block) { successors.push_back(block); }
  std::vector<MachineBasicBlock *> &get_successors() { return successors; }
  std::vector<MachineBasicBlock *> &get_predecessors() { return predecessors; }
  [[nodiscard]] size_t get_id() const { return id; }
};

struct CallingConvention {
private:
  std::string name;
  std::vector<PhysicalRegister> argument_regs;
  std::vector<PhysicalRegister> callee_saved;
  std::vector<PhysicalRegister> caller_saved;
};
struct MachineFunction {
private:
  MachineBasicBlock *entry_block;
  // Maybe later: CallingConvention calling_convention;
  std::size_t frame_size;
  std::size_t id;
  inline static std::size_t fn_id_counter = 0;

public:
  MachineFunction(MachineBasicBlock *entry_block, std::size_t frame_size)
      : entry_block(entry_block), frame_size(frame_size), id(fn_id_counter++) {}
  [[nodiscard]] MachineBasicBlock *get_entry_block() const {
    return entry_block;
  }
  [[nodiscard]] std::size_t get_id() const { return id; }
  bool operator==(const MachineFunction &other) const { return id == other.id; }

  [[nodiscard]] size_t get_frame_size() const {
    return frame_size;
  }
  void set_frame_size(size_t size) {
    frame_size = size;
  }
};

} // namespace mir

namespace std {
template <> struct hash<mir::MachineFunction> {
  std::size_t operator()(const mir::MachineFunction &p) const {
    return std::hash<size_t>()(p.get_id());
  }
};
}

namespace mir {

struct MIRProgram {
private:
  std::unordered_map<size_t, MachineFunction> functions{};

public:
  void add_function(MachineFunction func) { functions.emplace(func.get_id(), func); }
  [[nodiscard]] std::unordered_map<size_t, MachineFunction> &get_functions() {
    return functions;
  }
};

inline std::string to_string(const Register &reg) { return reg.get_name(); }

inline std::string to_string(const StackSlot &slot) {
  return std::format("stack[{}]",
                     slot.offset);
}

inline std::string to_string(const Immediate &imm) {
  return std::format("#{}", imm.value);
}

inline std::string to_string(const MemoryAccess &mem) {
  return std::format("[{}+{}]", to_string(mem.base_register), mem.offset);
}

inline std::string to_string(const MachineOperand &op) {
  return std::visit([](auto &&arg) -> std::string { return to_string(arg); },
                    op.get_op());
}

inline std::string to_string(MachineInstruction::MachineOpcode opcode) {
  switch (opcode) {
  case MachineInstruction::MachineOpcode::MOV_RR:
    return "MOV_RR";
  case MachineInstruction::MachineOpcode::MOV_RI:
    return "MOV_RI";
  case MachineInstruction::MachineOpcode::STORE_MEM_REG:
    return "STORE_MEM_REG";
  case MachineInstruction::MachineOpcode::LOAD_REG_MEM:
    return "LOAD_REG_MEM";
  case MachineInstruction::MachineOpcode::ADD_RR:
    return "ADD_RR";
  case MachineInstruction::MachineOpcode::ADD_RI:
    return "ADD_RI";
  case MachineInstruction::MachineOpcode::DIV_RR:
    return "DIV_RR";
  case MachineInstruction::MachineOpcode::DIV_RI:
    return "DIV_RI";
  case MachineInstruction::MachineOpcode::MUL_RR:
    return "MUL_RR";
  case MachineInstruction::MachineOpcode::MUL_RI:
    return "MUL_RI";
  case MachineInstruction::MachineOpcode::NEG_R:
    return "NEG_R";
  case mir::MachineInstruction::MachineOpcode::RET:
    return "RET";
  case MachineInstruction::MachineOpcode::SUB_RR:
    return "SUB";
  case MachineInstruction::MachineOpcode::SUB_RI:
    return "SUB";
  case MachineInstruction::MachineOpcode::MOD_RR:
    return "MOD";
  case MachineInstruction::MachineOpcode::MOD_RI:
    return "MOD";
  }
  return "UNKNOWN";
}

inline std::string to_string(const MachineInstruction &instr) {
  std::ostringstream oss;
  oss << to_string(instr.get_opcode());

  auto print_operands = [&](const std::vector<MachineOperand> &ops,
                            const std::string &prefix) {
    if (!ops.empty()) {
      oss << " " << prefix << ":";
      for (const auto &op : ops) {
        oss << " " << to_string(op);
      }
    }
  };

  print_operands(instr.get_outs(), "out");
  print_operands(instr.get_ins(), "in");
  print_operands(instr.get_implicit_defs(), "impl_def");
  print_operands(instr.get_implicit_uses(), "impl_use");

  return oss.str();
}

inline std::string to_string(const MachineBasicBlock &block) {
  std::ostringstream oss;
  oss << "Block " << block.get_id() << ":\n";
  for (const auto *instr : block.instructions) {
    oss << "  " << to_string(*instr) << "\n";
  }
  return oss.str();
}

inline std::string to_string(const MachineFunction &func) {
  std::ostringstream oss;
  oss << "Function " << func.get_id() << ":\n";
  std::unordered_set<MachineBasicBlock *> visited;
  std::list<MachineBasicBlock *> queue{};

  if (auto *entry = func.get_entry_block(); entry) {
    queue.push_front(entry);
    visited.insert(entry);
  }

  while (!queue.empty()) {
    MachineBasicBlock *block = queue.front();
    queue.pop_front();
    oss << to_string(*block);

    for (auto *succ : block->get_successors()) {
      if (visited.insert(succ).second) {
        queue.push_front(succ);
      }
    }
  }

  return oss.str();
}

inline std::string to_string(MIRProgram &program) {
  std::ostringstream oss;
  for (auto &func : program.get_functions()) {
    oss << to_string(func.second) << "\n";
  }
  return oss.str();
}

} // namespace mir
#endif // !MIR_MIR_H
