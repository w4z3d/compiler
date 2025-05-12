#ifndef COMPILER_CFG_H
#define COMPILER_CFG_H

#include "ir.hpp"
#include <optional>
#include <vector>

struct BasicBlock {
private:
  std::size_t block_id;
  std::vector<IRInstruction> instructions{};
  std::optional<BasicBlock *> successor_true;
  std::optional<BasicBlock *> successor_false;

public:
  explicit BasicBlock(std::size_t block_id)
      : block_id(block_id), successor_false(std::nullopt),
        successor_true(std::nullopt) {}
  [[nodiscard]] const std::vector<IRInstruction> &getInstructions() const {
    return instructions;
  }
  void add_instruction(const IRInstruction &instruction) {
    instructions.push_back(instruction);
  }
  [[nodiscard]] const std::optional<BasicBlock *> &getSuccessorTrue() const {
    return successor_true;
  }
  void setSuccessorTrue(const std::optional<BasicBlock *> &successorTrue) {
    successor_true = successorTrue;
  }
  [[nodiscard]] const std::optional<BasicBlock *> &getSuccessorFalse() const {
    return successor_false;
  }
  void setSuccessorFalse(const std::optional<BasicBlock *> &successorFalse) {
    successor_false = successorFalse;
  }
  [[nodiscard]] size_t getBlockId() const { return block_id; }

  [[nodiscard]] std::string to_string() const {
    std::stringstream out{};
    out << "Block id: " << block_id << std::endl;
    out << "Instructions: " << std::endl;

    for (const auto &inst : instructions) {
      out << inst.to_string() << std::endl;
    }

    out << "Successors: " << std::endl;
    if (successor_true.has_value()) {
      out << successor_true.value()->block_id << std::endl;
    }
    if (successor_false.has_value()) {
      out << successor_true.value()->block_id << std::endl;
    }
    return out.str();
  }
};

struct CFG {
  BasicBlock *entry_block;
  std::vector<BasicBlock *> exit_blocks;
  std::vector<std::vector<Var *>> live_vars;

public:
  explicit CFG(BasicBlock *entry_block) : entry_block(entry_block) {}
  [[nodiscard]] std::string to_string() const {
    std::stringstream out;
    out << entry_block->to_string() << std::endl;
    return out.str();
  }
  [[nodiscard]] BasicBlock *get_entry_block() const { return entry_block; }
};

struct IntermediateRepresentation {
private:
  std::vector<CFG> cfgs{};

public:
  IntermediateRepresentation() = default;

  void add_cfg(const CFG &cfg) { cfgs.push_back(cfg); }

  [[nodiscard]] std::string to_string() {
    std::ostringstream stream;
    for (const auto &cfg : cfgs) {
      stream << cfg.to_string() << std::endl;
    }

    return stream.str();
  }
};

#endif // COMPILER_CFG_H
