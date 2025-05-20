#ifndef COMPILER_CFG_H
#define COMPILER_CFG_H

#include "ir.hpp"
#include <optional>
#include <queue>
#include <set>
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
  [[nodiscard]] const std::vector<IRInstruction> &get_instructions() const {
    return instructions;
  }
  void add_instruction(const IRInstruction &instruction) {
    instructions.push_back(instruction);
  }
  [[nodiscard]] const std::optional<BasicBlock *> &get_successor_true() const {
    return successor_true;
  }
  void set_successor_true(const std::optional<BasicBlock *> &successorTrue) {
    successor_true = successorTrue;
  }
  [[nodiscard]] const std::optional<BasicBlock *> &get_successor_false() const {
    return successor_false;
  }
  void set_successor_false(const std::optional<BasicBlock *> &successorFalse) {
    successor_false = successorFalse;
  }
  [[nodiscard]] size_t get_id() const { return block_id; }

  [[nodiscard]] std::string to_string() const {
    std::stringstream out{};
    out << "Block id: " << block_id << std::endl; //
    out << "Instructions: " << std::endl;         //

    for (const auto &inst : instructions) {         //
      out << "  " << inst.to_string() << std::endl; //
    }

    out << "Successors: " << std::endl; //
    if (successor_true.has_value()) {   //
      // Using get_id() for consistency, assuming block_id itself isn't directly
      // public for successors
      out << "  True  -> Block " << successor_true.value()->get_id()
          << std::endl; //
    } else {
      out << "  True  -> None" << std::endl;
    }
    if (successor_false.has_value()) { //
      out << "  False -> Block " << successor_false.value()->get_id()
          << std::endl; //
    } else {
      out << "  False -> None" << std::endl;
    }
    return out.str();
  }

  // Getter for non-const access to instructions if needed by other parts (e.g.
  // peephole)
  std::vector<IRInstruction> &get_instructions() { //
    return instructions;                           //
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
    if (entry_block) { //
      // To print the whole CFG, a traversal would be needed here.
      // For now, keeping it simple as per original, or use to_dot_graph().
      out << to_dot_graph() << std::endl; //
    } else {
      out << "CFG is empty (no entry block)." << std::endl;
    }
    return out.str();
  }
  [[nodiscard]] const BasicBlock *get_entry_block() const {
    return entry_block;
  } //

  [[nodiscard]] std::string to_dot_graph() const {
    std::stringstream dot_graph;
    dot_graph << "digraph CFG {\n";
    dot_graph << "  rankdir=TB;\n"; // Top-to-bottom layout
    dot_graph << "  node [shape=box, fontname=\"Courier New\", fontsize=10];\n";
    dot_graph << "  edge [fontname=\"Arial\", fontsize=9];\n";

    std::set<const BasicBlock *> visited_blocks;
    std::queue<const BasicBlock *> worklist;

    if (entry_block) {            //
      worklist.push(entry_block); //
      visited_blocks.insert(entry_block);
    }

    while (!worklist.empty()) {
      const BasicBlock *current_bb = worklist.front();
      worklist.pop();

      // Define the node for the current block
      dot_graph << "  \"bb_" << current_bb->get_id() << "\" [label=\""; //
      dot_graph << "Block " << current_bb->get_id() << "\\n\\n";        //
      for (const auto &instr : current_bb->get_instructions()) {        //
        std::string instr_str = instr.to_string();                      //
        // Escape characters for DOT label string
        std::string escaped_instr_str;
        for (char c : instr_str) {
          if (c == '"')
            escaped_instr_str += "\\\"";
          else if (c == '\\')
            escaped_instr_str += "\\\\";
          else if (c == '\n')
            escaped_instr_str += "\\l"; // DOT newline (left-justified)
          else
            escaped_instr_str += c;
        }
        dot_graph << escaped_instr_str
                  << "\\l"; // Add left-justified newline after each instruction
      }
      dot_graph << "\", peripheries="
                << (exit_blocks.end() != std::find(exit_blocks.begin(),
                                                   exit_blocks.end(),
                                                   current_bb)
                        ? "2"
                        : "1")
                << "];\n";

      // Edge for successor_true
      if (current_bb->get_successor_true().has_value()) { //
        const BasicBlock *succ_true =
            current_bb->get_successor_true().value(); //
        dot_graph << "  \"bb_" << current_bb->get_id() << "\" -> \"bb_"
                  << succ_true->get_id() << "\"";            //
        if (current_bb->get_successor_false().has_value()) { //
          dot_graph << " [label=\" True\", color=\"darkgreen\", "
                       "fontcolor=\"darkgreen\"];\n";
        } else {
          dot_graph << " [color=\"blue\"];\n"; // Unconditional
        }

        if (visited_blocks.find(succ_true) == visited_blocks.end()) {
          visited_blocks.insert(succ_true);
          worklist.push(succ_true);
        }
      }

      // Edge for successor_false
      if (current_bb->get_successor_false().has_value()) { //
        const BasicBlock *succ_false =
            current_bb->get_successor_false().value(); //
        dot_graph
            << "  \"bb_" << current_bb->get_id() << "\" -> \"bb_"
            << succ_false->get_id()
            << "\" [label=\" False\", color=\"red\", fontcolor=\"red\"];\n"; //

        if (visited_blocks.find(succ_false) == visited_blocks.end()) {
          visited_blocks.insert(succ_false);
          worklist.push(succ_false);
        }
      }
    }
    // Mark exit blocks distinctly if they weren't handled by peripheries (e.g.
    // if not reachable) The current peripheries method handles reachable exit
    // blocks.

    dot_graph << "}\n";
    return dot_graph.str();
  };
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

  [[nodiscard]] std::vector<CFG> &get_cfgs() { return cfgs; }
};

#endif // COMPILER_CFG_H
