#ifndef COMPILER_LIVENESS_H
#define COMPILER_LIVENESS_H

#include "../ir/cfg.hpp"
#include "../mir/mir.hpp"

class Liveness {
private:
  size_t id_counter = 0;
  mir::MIRProgram &mir_program;
  std::unordered_map<size_t, std::list<std::unordered_set<size_t>>>
      block_to_live{};
  std::vector<mir::MachineBasicBlock *> basic_block_order{};
  std::unordered_map<std::string, size_t> physical_to_live_id{};
  std::unordered_map<size_t, std::string> live_id_to_physical{};
  std::unordered_map<size_t, size_t> virtual_to_live_id{};
  std::unordered_map<size_t, size_t> live_id_to_virtual{};

  void analyse_func(const mir::MachineFunction &machine_function);
  void dfs_basic_block(mir::MachineBasicBlock *bb,
                       std::unordered_set<size_t> &visited);

public:
  explicit Liveness(mir::MIRProgram &mir_program) : mir_program(mir_program) {}
  // 1. Create mapping from virtual and physical register to size_t
  // 2. Analyse liveness
  // 3. provide adapter to get liveness information
  void analyse();
  std::string to_string_block_to_live() {
    std::ostringstream oss;
    for (const auto &[block_id, live_sets] : block_to_live) {
      oss << "Block " << block_id << ":\n";
      int i = 1;
      for (const auto &live_set : live_sets) {
        oss << "  Line " << i++ << ": {";
        bool first = true;
        for (const auto &var : live_set) {
          if (!first)
            oss << ", ";
          else
            first = false;
          if (live_id_to_virtual.contains(var)) {
            oss << live_id_to_virtual[var];
          } else {
            oss << live_id_to_physical[var];
          }
        }
        oss << "}\n";
      }
    }
    return oss.str();
  }
};

#endif // COMPILER_LIVENESS_H
