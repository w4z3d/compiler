#include "liveness.hpp"
#include <algorithm>
#include <ranges>

namespace liveness {

unsigned count_vregs(lir::Function *func) {
  unsigned max_id = 0;
  bool found = false;
  for (auto &reg : func->param_regs) {
    if (reg.is_virtual()) {
      max_id = std::max(max_id, reg.id);
      found = true;
    }
  }
  for (auto *mbb : func->blocks) {
    for (auto *inst : mbb->instructions) {
      for (auto &op : inst->operands) {
        if (op.is_reg() && op.get_reg().is_virtual()) {
          max_id = std::max(max_id, op.get_reg().id);
          found = true;
        }
      }
    }
  }
  return found ? max_id + 1 : 0;
}

// Collect all vreg defs in a block into a BitSet
static BitSet compute_block_defs(lir::BasicBlock *mbb, unsigned num_vregs) {
  BitSet defs(num_vregs);
  for (auto *inst : mbb->instructions) {
    for (auto &op : inst->defs()) {
      if (op.is_reg() && op.get_reg().is_virtual())
        defs.set(op.get_reg().id);
    }
  }
  return defs;
}

// Collect all vreg uses in a block that are not preceded by a def in the same
// block (upward-exposed uses)
static BitSet compute_block_uses(lir::BasicBlock *mbb, unsigned num_vregs) {
  BitSet uses(num_vregs);
  BitSet defs(num_vregs);
  for (auto *inst : mbb->instructions) {
    // Uses before defs at this instruction
    for (auto &op : inst->uses()) {
      if (op.is_reg() && op.get_reg().is_virtual()) {
        if (!defs.test(op.get_reg().id))
          uses.set(op.get_reg().id);
      }
    }
    for (auto &op : inst->defs()) {
      if (op.is_reg() && op.get_reg().is_virtual())
        defs.set(op.get_reg().id);
    }
  }
  return uses;
}

LivenessInfo compute_liveness(lir::Function *func) {
  unsigned num_vregs = count_vregs(func);
  LivenessInfo info;
  info.num_vregs = num_vregs;

  if (num_vregs == 0)
    return info;

  // Precompute per-block defs and uses
  std::unordered_map<lir::BasicBlock *, BitSet> block_defs;
  std::unordered_map<lir::BasicBlock *, BitSet> block_uses;

  for (auto *mbb : func->blocks) {
    block_defs.emplace(mbb, compute_block_defs(mbb, num_vregs));
    block_uses.emplace(mbb, compute_block_uses(mbb, num_vregs));
    info.live_in.emplace(mbb, BitSet(num_vregs));
    info.live_out.emplace(mbb, BitSet(num_vregs));
  }

  // Parameters are live-in at entry block
  for (auto &param : func->param_regs) {
    if (param.is_virtual() && param.id < num_vregs) {
      info.live_in.at(func->entry_block()).set(param.id);
    }
  }

  // Fixed-point iteration
  // live_out[B] = union of live_in[S] for all successors S of B
  // live_in[B]  = uses[B] | (live_out[B] & ~defs[B])
  bool changed = true;
  while (changed) {
    changed = false;

    for (auto mbb : std::ranges::reverse_view(func->blocks)) {
      // Compute new live_out: union of all successors' live_in
      BitSet new_live_out(num_vregs);
      for (auto *succ : mbb->successors) {
        new_live_out |= info.live_in.at(succ);
      }

      // Compute new live_in: uses | (live_out & ~defs)
      // Which is: uses | (live_out - defs)
      BitSet live_through(new_live_out);
      // live_through &= ~defs  →  we need to clear bits that are in defs
      BitSet not_defs(block_defs.at(mbb));
      // Flip all bits, then AND with live_out
      // But BitSet doesn't have NOT, so do it manually:
      // live_through = live_out with def bits cleared
      for (auto def_bit : block_defs.at(mbb)) {
        live_through.reset(def_bit);
      }
      BitSet new_live_in(block_uses.at(mbb));
      new_live_in |= live_through;

      // Check for changes by comparing with previous values
      // We can't compare BitSets directly with !=, so check if OR changes
      // anything
      bool out_changed = false;
      bool in_changed = false;

      // Check live_out changed
      BitSet out_diff(new_live_out);
      out_diff ^= info.live_out.at(mbb);
      if (out_diff.count() > 0)
        out_changed = true;

      BitSet in_diff(new_live_in);
      in_diff ^= info.live_in.at(mbb);
      if (in_diff.count() > 0)
        in_changed = true;

      if (out_changed || in_changed) {
        changed = true;
        info.live_out.at(mbb) = std::move(new_live_out);
        info.live_in.at(mbb) = std::move(new_live_in);
      }
    }
  }

  return info;
}

void build_interference_graph(lir::Function *func, LivenessInfo &info,
                              UndirectedGraph &graph) {
  unsigned num_vregs = info.num_vregs;
  if (num_vregs == 0)
    return;

  for (const auto &reg : func->param_regs) {
    for (const auto &reg2 : func->param_regs) {
      if (reg.id != reg2.id)
        graph.add_edge(reg.id, reg2.id);
    }
  }
  for (auto *mbb : func->blocks) {
    // Start with live_out as a BitSet
    BitSet live(info.live_out.at(mbb));

    // Walk instructions backwards
    for (auto inst : std::ranges::reverse_view(mbb->instructions)) {
      // Add uses to live set first
      for (auto &op : inst->uses()) {
        if (op.is_reg() && op.get_reg().is_virtual())
          live.set(op.get_reg().id);
      }

      // Each def interferes with everything currently live
      for (auto &def_op : inst->defs()) {
        if (!def_op.is_reg() || !def_op.get_reg().is_virtual())
          continue;
        unsigned def_id = def_op.get_reg().id;

        // Add edges between def and all live registers
        for (auto live_id : live) {
          if (live_id != def_id)
            graph.add_edge(def_id, live_id);
        }
      }

      // Remove defs from live set
      for (auto &op : inst->defs()) {
        if (op.is_reg() && op.get_reg().is_virtual())
          live.reset(op.get_reg().id);
      }
    }
  }
}

} // namespace liveness
