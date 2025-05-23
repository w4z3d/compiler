#ifndef COMPILER_LIVENESS_H
#define COMPILER_LIVENESS_H

#include "../code_gen/yapper.hpp"
#include "../ir/cfg.hpp"
#include "../mir/mir.hpp"

class Liveness {
private:
  size_t id_counter = 0;
  mir::MIRProgram &mir_program;
  MIRRegisterMap &rmap;
  std::unordered_map<size_t, std::list<std::unordered_set<size_t>>>
      block_to_live{};

  void analyse_func(const mir::MachineFunction &machine_function);

public:
  explicit Liveness(mir::MIRProgram &mir_program, MIRRegisterMap &rmap)
      : mir_program(mir_program), rmap(rmap) {}
  // 1. Create mapping from virtual and physical register to size_t
  // 2. Analyse liveness
  // 3. provide adapter to get liveness information
  void analyse();
  std::unordered_map<size_t, std::list<std::unordered_set<size_t>>> &
  get_liveness() {
    return block_to_live;
  }
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
          if (rmap.virtual_from_live(var).has_value()) {
            oss << rmap.virtual_from_live(var).value();
          } else {
            oss << rmap.physical_from_live(var).value();
          }
        }
        oss << "}\n";
      }
    }
    return oss.str();
  }
};

#endif // COMPILER_LIVENESS_H
