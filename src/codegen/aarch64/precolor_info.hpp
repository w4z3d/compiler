#include "aarch64_defs.hpp"
std::vector<std::pair<size_t, size_t>> precolor(lir::Function *func) {
  const auto &target = aarch64::target;

  std::vector<std::pair<size_t, size_t>> precolored;
  precolored.reserve(func->param_regs.size());
  for (size_t i = 0; i < func->param_regs.size(); i++) {
    precolored.emplace_back(func->param_regs[i].id, target.arg_regs[i].id);
  }

  return precolored;
}
