#ifndef SRC_CODEGEN_REGALLOC_HPP
#define SRC_CODEGEN_REGALLOC_HPP
#include "../graph_coloring/graph_coloring.hpp"
#include "../lir/lir.hpp"

#include <unordered_map>

namespace regalloc {
struct CoalesceInfo {
  // Maps merged vreg → representative vreg
  std::unordered_map<unsigned, unsigned> representative;

  unsigned find(unsigned v) {
    while (representative.count(v))
      v = representative[v];
    return v;
  }
};

CoalesceInfo coalesce(lir::Function *func, UndirectedGraph &graph);

void rewrite_registers(lir::Function *func,
                       std::unordered_map<size_t, size_t> &coloring);
} // namespace regalloc
#endif
