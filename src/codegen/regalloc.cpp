#include "regalloc.hpp"
namespace regalloc {
CoalesceInfo coalesce(lir::Function *func, UndirectedGraph &graph) {
  CoalesceInfo info;

  // Collect all copy instructions
  std::vector<lir::Instruction *> copies;
  for (auto *mbb : func->blocks) {
    for (auto *inst : mbb->instructions) {
      if (inst->opcode != lir::Opcode::Copy)
        continue;
      if (!inst->def(0).is_reg() || !inst->use(0).is_reg())
        continue;
      if (!inst->def(0).get_reg().is_virtual())
        continue;
      if (!inst->use(0).get_reg().is_virtual())
        continue;
      copies.push_back(inst);
    }
  }

  for (auto *inst : copies) {
    unsigned dst = info.find(inst->def(0).get_reg().id);
    unsigned src = info.find(inst->use(0).get_reg().id);

    if (dst == src)
      continue; // already coalesced

    // Conservative check: don't coalesce if they interfere
    if (graph.get_adjacent_list().has_edge(dst, src))
      continue;

    // Merge src into dst: all edges of src become edges of dst
    auto &adj = graph.get_adjacent_list();
    for (auto neighbor : adj[src]) {
      if (neighbor != dst)
        graph.add_edge(dst, neighbor);
    }

    // Remove src from graph
    // (clear its edges so it doesn't participate in coloring)
    // We keep the node but it will get dst's color via representative map
    info.representative[src] = dst;
  }

  return info;
}

void rewrite_registers(lir::Function *func,
                       std::unordered_map<size_t, size_t> &coloring) {
  for (auto &reg : func->param_regs) {
    if (reg.is_virtual()) {
      reg = lir::Register::preg(coloring[reg.id]);
    }
  }
  for (auto *mbb : func->blocks) {
    for (auto *inst : mbb->instructions) {
      for (auto &op : inst->operands) {
        if (!op.is_reg() || !op.get_reg().is_virtual())
          continue;
        unsigned color = coloring[op.get_reg().id];
        op = lir::Operand::from_reg(lir::Register::preg(color));
      }
    }

    // Remove copies where src == dst (coalesced away)
    auto &instrs = mbb->instructions;
    instrs.erase(std::remove_if(instrs.begin(), instrs.end(),
                                [](lir::Instruction *inst) {
                                  if (!inst->is_copy() ||
                                      !inst->operands[0].is_reg() ||
                                      !inst->operands[1].is_reg())
                                    return false;
                                  return inst->operands[0].get_reg() ==
                                         inst->operands[1].get_reg();
                                }),
                 instrs.end());
  }
}
} // namespace regalloc
