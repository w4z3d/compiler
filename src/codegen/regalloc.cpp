#include "regalloc.hpp"
namespace regalloc {

// Briggs criterion: safe to coalesce if the merged node
// has fewer than k neighbors of significant degree (degree >= k)
static bool briggs_safe(UndirectedGraph &graph, unsigned a, unsigned b,
                        unsigned k) {
  auto &adj = graph.get_adjacent_list();
  std::unordered_set<unsigned> combined_neighbors;

  for (auto n : adj[a]) {
    if (n != b)
      combined_neighbors.insert(n);
  }
  for (auto n : adj[b]) {
    if (n != a)
      combined_neighbors.insert(n);
  }

  unsigned significant = 0;
  for (auto n : combined_neighbors) {
    if (adj[n].count() >= k)
      significant++;
  }

  return significant < k;
}

// George criterion: safe to coalesce virtual A into physical B if
// every neighbor of A either already interferes with B
// or has degree < k
static bool george_safe(UndirectedGraph &graph, unsigned vreg, unsigned preg,
                        unsigned k) {
  auto &adj = graph.get_adjacent_list();
  for (auto neighbor : adj[vreg]) {
    if (neighbor == preg)
      continue;
    bool interferes_with_preg = adj.has_edge(neighbor, preg);
    bool insignificant = adj[neighbor].count() < k;
    if (!interferes_with_preg && !insignificant)
      return false;
  }
  return true;
}

CoalesceInfo coalesce(lir::Function *func, UndirectedGraph &graph,
                      unsigned num_allocatable) {
  CoalesceInfo info;

  struct CopyRecord {
    unsigned dst_id;
    unsigned src_id;
    bool dst_physical;
    bool src_physical;
  };

  std::vector<CopyRecord> copies;
  for (auto *mbb : func->blocks) {
    for (auto *inst : mbb->instructions) {
      if (inst->opcode != lir::Opcode::Copy)
        continue;
      if (!inst->def(0).is_reg() || !inst->use(0).is_reg())
        continue;

      auto dst = inst->def(0).get_reg();
      auto src = inst->use(0).get_reg();

      if (dst.is_physical() && src.is_physical())
        continue;

      copies.push_back({dst.id, src.id, dst.is_physical(), src.is_physical()});
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto &copy : copies) {
      unsigned dst = info.find(copy.dst_id);
      unsigned src = info.find(copy.src_id);

      if (dst == src)
        continue;

      if (graph.get_adjacent_list().has_edge(dst, src))
        continue;

      bool dst_is_phys = copy.dst_physical || dst != copy.dst_id;
      bool src_is_phys = copy.src_physical || src != copy.src_id;

      bool safe = false;
      if (dst_is_phys && !src_is_phys) {
        safe = george_safe(graph, src, dst, num_allocatable);
      } else if (!dst_is_phys && src_is_phys) {
        safe = george_safe(graph, dst, src, num_allocatable);
      } else if (!dst_is_phys && !src_is_phys) {
        safe = briggs_safe(graph, dst, src, num_allocatable);
      }

      if (!safe)
        continue;

      unsigned keep = dst_is_phys ? dst : (src_is_phys ? src : dst);
      unsigned remove = (keep == dst) ? src : dst;

      graph.merge_nodes(keep, remove);

      info.representative[remove] = keep;
      changed = true;
    }
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
        if (!coloring.contains(op.get_reg().id)) {
          throw std::runtime_error("No spilling implemented lmao");
        }

        unsigned color = coloring[op.get_reg().id];
        auto clazz = op.get_reg().get_class();
        auto reg = lir::Register::preg(color);
        op = lir::Operand::from_reg(reg);
        op.get_reg_mut().set_class(clazz);
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
