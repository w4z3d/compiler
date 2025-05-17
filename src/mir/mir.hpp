#ifndef MIR_MIR_H
#define MIR_MIR_H

#include <cstddef>
#include <cstdint>
#include <format>
#include <list>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace mir {

class Register;

struct StackSlot {
  Register &stack_pointer;
  std::size_t offset;
};

struct Immediate {
  std::int32_t value;
};

struct MemoryAccess {
  Register &base_register;
  std::size_t offset;
};

class Register {
private:
  std::string name;
  int bit_size;

public:
  Register(std::string name, int bit_size)
      : name(std::move(name)), bit_size(bit_size) {}
  [[nodiscard]] std::string get_name() { return name; }
};

class VirtualRegister : public Register {
private:
  std::size_t numeral;

public:
  VirtualRegister(std::size_t numeral, int bit_size)
      : Register(std::format("vreg{}", numeral), bit_size), numeral(numeral) {}
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
};

struct MachineInstruction {
  enum class MachineOpcode {
    MOV_REG_REG,
    MOV_REG_IMM,
    STORE_MEM_REG,
    LOAD_REG_MEM,

    ADD_RR, // inout:dest += in:src
    ADD_RI, // inout:dest += in:imm

    DIV_RR, // out:dst = in:dst / in:src out:rem = in:dst % in:src
    DIV_RI, // out:dst = in:dst / in:imm out:rem = in:dst % in:src

    MUL_RR, // inout:dest *= in:src
    MUL_RI, // inout:dest *= in:imm

    NEG_R, // out:dest = -in:dest

  };

  MachineOpcode opcode;
  std::vector<MachineOperand> ins; 
  std::vector<MachineOperand> outs;
  std::vector<MachineOperand> implicit_defs;
  std::vector<MachineOperand> implicit_uses;
};

struct MachineBasicBlock {
  std::list<MachineInstruction> instructions{};
  std::vector<MachineBasicBlock *> successors{};
  std::vector<MachineBasicBlock *> predecessors{};

  std::size_t id;
  static std::size_t id_counter;

public:
  MachineBasicBlock() : id(id_counter++) {}

  void add_instruction(const MachineInstruction &instruction) {
    instructions.push_back(instruction);
  }

  void add_successor(MachineBasicBlock *block) { successors.push_back(block); }
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

public:
  MachineFunction(MachineBasicBlock *entry_block, std::size_t frame_size)
      : entry_block(entry_block), frame_size(frame_size) {}
};

struct MIRProgram {
private:
  std::vector<MachineFunction> functions{};

public:
  void add_function(const MachineFunction &func) { functions.push_back(func); }
};
} // namespace mir
#endif // !MIR_MIR_H
