#ifndef COMPILER_LIVENESS_H
#define COMPILER_LIVENESS_H

#include "../ir/cfg.hpp"

class Liveness {
private:
  IntermediateRepresentation &intermediateRepresentation;
  // size_t is block_id
  std::unordered_map<size_t, std::vector<std::unordered_set<Var>>> block_to_live{};
  void analyse_cfg(const CFG &cfg);
public:
  explicit Liveness(IntermediateRepresentation &repr)
      : intermediateRepresentation(repr) {}
  void analyse();
  std::unordered_map<size_t, std::vector<std::unordered_set<Var>>> &get_42() {
    return block_to_live;
  }
  std::string to_string_block_to_live() {
    std::ostringstream oss;

    for (const auto& [block_id, live_sets] : block_to_live) {
      oss << "Block " << block_id << ":\n";

      // Beachte: vector[0] ist die letzte Zeile, also rückwärts iterieren
      for (size_t i = 0; i < live_sets.size(); ++i) {
        size_t line_index = live_sets.size() - 1 - i;
        const auto& live_set = live_sets[line_index];

        oss << "  Line " << i << ": {";
        bool first = true;
        for (const auto& var : live_set) {
          if (!first) oss << ", ";
          else first = false;

          oss << var.to_string();
        }
        oss << "}\n";
      }
    }

    return oss.str();
  }
};

#endif // COMPILER_LIVENESS_H
