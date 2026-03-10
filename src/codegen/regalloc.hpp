#ifndef SRC_CODEGEN_REGALLOC_HPP
#define SRC_CODEGEN_REGALLOC_HPP
#include "../graph_coloring/graph_coloring.hpp"
#include "../lir/lir.hpp"

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
                       std::unordered_map<size_t, size_t> &coloring);
} // namespace regalloc
#endif
