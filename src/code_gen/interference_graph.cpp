#include "interference_graph.hpp"

void InterferenceGraph::construct() {
  for (const auto &[block_id, liveness] : block_to_live) {
    for (const auto &line : liveness) {
      for (const auto &var : line) {
        // var already mapped to neighbour set
        if (adjacent_map.contains(var)) {
          std::unordered_set<Var> &var_neighbours = adjacent_map[var];
          for (const auto &item : line) {
            if (item == var)
              continue;
            var_neighbours.insert(item);
          }
        } else {
          std::unordered_set<Var> var_neighbours{};
          for (const auto &item : line) {
            if (item == var)
              continue;
            var_neighbours.insert(item);
          }
          adjacent_map[var] = var_neighbours;
        }
      }
    }
  }
}
