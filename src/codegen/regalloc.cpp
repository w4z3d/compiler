#include "regalloc.hpp"
#include "../lir/lir_builder.hpp"
#include "aarch64/aarch64_defs.hpp"
#include "aarch64/precolor_info.hpp"
#include "target.hpp"
namespace regalloc {

std::vector<unsigned>
choose_spill_candidate(lir::Function *func,
                       const std::unordered_map<size_t, size_t> &coloring,
                       const std::vector<std::pair<size_t, size_t>> &precolored,
                       CoalesceInfo &coalesce_info, TargetInfo target) {

  std::unordered_set<unsigned> seen;
  std::vector<unsigned> candidates;

  for (auto *bb : func->blocks) {
    for (auto *inst : bb->instructions) {
      for (auto &op : inst->operands) {
        if (!op.is_reg())
          continue;
        auto reg = op.get_reg();
        if (!reg.is_virtual())
          continue;

        unsigned id = reg.id;
        size_t root = coalesce_info.find(id);

        if (seen.contains(root))
          continue;
        seen.insert(root);

        auto it = coloring.find(root);
        if (it == coloring.end()) {
        } else {
          size_t color = it->second;
          if (color >= target.num_allocatable) {
            candidates.push_back(root);
          }
        }
      }
    }
  }

  return candidates;
}
// Register allocate and spilling. rerun coloring if spilling
void allocate_registers(lir::Module &module, TargetInfo target) {
  for (auto *func : module.functions) {
    if (func->is_extern)
      continue;

    while (true) {
      auto info = liveness::compute_liveness(func);

      UndirectedGraph graph(info.num_vregs);
      liveness::build_interference_graph(func, info, graph);

      auto precolored = precolor(func, target);

      auto coalesce_info =
          regalloc::coalesce(func, graph, target.num_allocatable);

      auto coloring = graph.color(precolored, info.num_vregs);

      // propagate coalesced colors
      for (auto &[removed, kept] : coalesce_info.representative) {
        unsigned root = coalesce_info.find(removed);
        if (coloring.contains(root)) {
          coloring[removed] = coloring[root];
        }
      }

      auto victim = choose_spill_candidate(func, coloring, precolored,
                                           coalesce_info, target);

      if (victim.empty()) {
        regalloc::rewrite_registers(func, coloring, target);
        break;
      }

      rewrite_spill(func, victim);
    }
  }
}
lir::Instruction *make_spill_load(lir::Register dst, size_t slot) {
  auto *load = new lir::Instruction();
  load->opcode = lir::Opcode::Load;
  load->num_defs = 1;
  load->operands.push_back(lir::Operand::from_reg(dst));
  load->operands.push_back(lir::Operand::from_slot(slot));
  return load;
}

lir::Instruction *make_spill_store(size_t slot, lir::Register src) {
  auto *store = new lir::Instruction();
  store->opcode = lir::Opcode::Store;
  store->num_defs = 0;
  store->operands.push_back(lir::Operand::from_slot(slot));
  store->operands.push_back(lir::Operand::from_reg(src));
  return store;
}
void rewrite_spill(lir::Function *func, const std::vector<unsigned> &victims) {
  for (const auto victim : victims) {
    auto slot = get_or_create_spill_slot(func, victim);

    for (auto *block : func->blocks) {
      std::vector<lir::Instruction *> rewritten;

      for (auto *inst : block->instructions) {
        std::unordered_map<size_t, lir::Register> loaded_temps;

        // maybe clone inst if needed
        auto *new_inst = inst;

        // rewrite uses
        for (size_t i = new_inst->num_defs; i < new_inst->operands.size();
             i++) {
          auto &op = new_inst->operands[i];
          if (!op.is_reg())
            continue;
          if (!op.get_reg().is_virtual())
            continue;
          if (op.get_reg().id != victim)
            continue;

          lir::Register tmp{};
          if (loaded_temps.contains(victim)) {
            tmp = loaded_temps[victim];
          } else {
            tmp = lir::Register::vreg(++func->next_vreg_id);
            tmp.set_class(op.get_reg().get_class());
            rewritten.push_back(make_spill_load(tmp, slot));
            loaded_temps[victim] = tmp;
          }

          op = lir::Operand::from_reg(tmp);
        }

        std::vector<lir::Register> def_temps;

        // rewrite defs
        for (size_t i = 0; i < new_inst->num_defs; i++) {
          auto &op = new_inst->operands[i];
          if (!op.is_reg())
            continue;
          if (!op.get_reg().is_virtual())
            continue;
          if (op.get_reg().id != victim)
            continue;

          auto tmp = lir::Register::vreg(++func->next_vreg_id);
          tmp.set_class(op.get_reg().get_class());
          op = lir::Operand::from_reg(tmp);
          def_temps.push_back(tmp);
        }

        rewritten.push_back(new_inst);

        for (auto tmp : def_temps) {
          rewritten.push_back(make_spill_store(slot, tmp));
        }
      }

      block->instructions = std::move(rewritten);
    }
  }
}

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
                       std::unordered_map<size_t, size_t> &coloring,
                       TargetInfo target) {
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
        auto clazz = op.get_reg().get_class();
        auto reg = target.allocatable[color];
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
