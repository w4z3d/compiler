#ifndef SRC_ANALYSIS_LIVENESS_HPP
#define SRC_ANALYSIS_LIVENESS_HPP
#include "../graph_coloring/graph_coloring.hpp"
#include "../lir/lir.hpp"
#include <unordered_map>
#include <unordered_set>

namespace std {
template <> struct hash<lir::Register> {
  size_t operator()(const lir::Register &r) const {
    return hash<unsigned>()(r.id) ^ (hash<uint8_t>()(r.kind) << 16);
  }
};
} // namespace std

namespace liveness {

struct LivenessInfo {
  // Per-block live sets using BitSets for fast set operations
  std::unordered_map<lir::BasicBlock *, BitSet> live_in;
  std::unordered_map<lir::BasicBlock *, BitSet> live_out;
  unsigned num_vregs = 0;
};

// Scans all instructions to find the number of virtual registers
unsigned count_vregs(lir::Function *func);

// Computes live_in and live_out for each block using fixed-point iteration
LivenessInfo compute_liveness(lir::Function *func);

// Builds interference graph from liveness info
// Walks each block backwards, adding edges between defs and live sets
void build_interference_graph(lir::Function *func, LivenessInfo &info,
                              UndirectedGraph &graph);

} // namespace liveness

#endif
