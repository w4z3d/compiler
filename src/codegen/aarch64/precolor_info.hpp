#include "aarch64_defs.hpp"
#include <set>
std::vector<std::pair<size_t, size_t>> precolor(lir::Function *func,
                                                TargetInfo target) {
  std::set<unsigned> seen;
  std::vector<std::pair<size_t, size_t>> precolored;

  auto allocatable_index = [&](unsigned phys_id) -> int {
    for (size_t i = 0; i < target.num_allocatable; i++) {
      if (target.allocatable[i].id == phys_id)
        return (int)i;
    }
    return -1; // not allocatable (sp, fp, lr, etc.)
  };

  for (auto *mbb : func->blocks) {
    for (auto *inst : mbb->instructions) {
      for (auto &op : inst->operands) {
        if (op.is_reg() && op.get_reg().is_physical()) {
          unsigned id = op.get_reg().id;
          if (!seen.insert(id).second)
            continue;
          int color = allocatable_index(id);
          if (color >= 0)
            precolored.emplace_back(id, (size_t)color);
        }
      }
      for (auto &reg : inst->implicit_defs) {
        if (reg.is_physical()) {
          unsigned id = reg.id;
          if (!seen.insert(id).second)
            continue;
          int color = allocatable_index(id);
          if (color >= 0)
            precolored.emplace_back(id, (size_t)color);
        }
      }
      for (auto &reg : inst->implicit_uses) {
        if (reg.is_physical()) {
          unsigned id = reg.id;
          if (!seen.insert(id).second)
            continue;
          int color = allocatable_index(id);
          if (color >= 0)
            precolored.emplace_back(id, (size_t)color);
        }
      }
    }
  }
  return precolored;
}
