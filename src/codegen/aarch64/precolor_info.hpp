#include "aarch64_defs.hpp"
#include <set>
std::vector<std::pair<size_t, size_t>> precolor(lir::Function *func) {
  std::set<unsigned> seen;
  std::vector<std::pair<size_t, size_t>> precolored;

  for (auto *mbb : func->blocks) {
    for (auto *inst : mbb->instructions) {
      // Explicit operands
      for (auto &op : inst->operands) {
        if (op.is_reg() && op.get_reg().is_physical()) {
          unsigned id = op.get_reg().id;
          if (seen.insert(id).second) {
            precolored.emplace_back(id, id);
          }
        }
      }
      // Implicit defs (caller-saved clobbered by calls)
      for (auto &reg : inst->implicit_defs) {
        if (reg.is_physical()) {
          unsigned id = reg.id;
          if (seen.insert(id).second) {
            precolored.emplace_back(id, id);
          }
        }
      }
      // Implicit uses
      for (auto &reg : inst->implicit_uses) {
        if (reg.is_physical()) {
          unsigned id = reg.id;
          if (seen.insert(id).second) {
            precolored.emplace_back(id, id);
          }
        }
      }
    }
  }

  return precolored;
}
