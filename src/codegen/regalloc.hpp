#ifndef SRC_CODEGEN_REGALLOC_HPP
#define SRC_CODEGEN_REGALLOC_HPP
#include "../alloc/arena.hpp"
#include "../analysis/liveness.hpp"
#include "../lir/lir.hpp"
#include "graph_coloring/graph_coloring.hpp"
#include "target.hpp"

#include <unordered_map>

namespace regalloc {
struct CoalesceInfo {
  std::unordered_map<unsigned, unsigned> representative;

  unsigned find(unsigned id) {
    if (!representative.contains(id))
      return id;
    // Path compression
    unsigned root = id;
    while (representative.contains(root))
      root = representative[root];
    // Compress path
    unsigned current = id;
    while (current != root) {
      unsigned next = representative[current];
      representative[current] = root;
      current = next;
    }
    return root;
  }
};

CoalesceInfo coalesce(lir::Function *func, UndirectedGraph &graph,
                      unsigned num_allocatable);

void rewrite_registers(lir::Function *func,
                       std::unordered_map<size_t, size_t> &coloring,
                       TargetInfo target);

std::vector<unsigned>
choose_spill_candidate(lir::Function *func,
                       const std::unordered_map<size_t, size_t> &coloring,
                       const std::unordered_map<size_t, size_t> &precolored,
                       CoalesceInfo &coalesce_info, TargetInfo target);
void allocate_registers(lir::Module &module, TargetInfo target);

void rewrite_spill(lir::Function *func, const std::vector<unsigned> &victim);
lir::Instruction *make_spill_load(lir::Register dst, size_t slot);
lir::Instruction *make_spill_store(size_t slot, lir::Register src);
inline size_t get_or_create_spill_slot(lir::Function *func, size_t vreg_id) {
  if (func->spill_slots.contains(vreg_id)) {
    return func->spill_slots[vreg_id];
  }

  size_t offset = func->next_spill_slot;
  func->next_spill_slot += 1;
  func->spill_slots[vreg_id] = offset;
  return offset;
}
} // namespace regalloc
#endif
