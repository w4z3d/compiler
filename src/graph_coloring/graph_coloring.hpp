#ifndef COMPILER_GRAPH_COLORING_H
#define COMPILER_GRAPH_COLORING_H

#include "../util/graph_helper.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct UndirectedGraph { // but is it really chordal :(
private:
  AdjacencyList adjacency_list;

  static size_t get_max_key(BitSet &set, std::vector<size_t> &weight);
  std::vector<size_t> maximum_cardinality_search(
      std::vector<size_t> &seo,
      const std::vector<std::pair<size_t, size_t>> &precolored_nodes,
      size_t num_nodes); // returns simplicial elimination ordering
  size_t get_color(size_t v, std::unordered_map<size_t, size_t> &var_to_color);
  void greedy_coloring(std::vector<size_t> &soe,
                       std::unordered_map<size_t, size_t> &var_to_color);

public:
  explicit UndirectedGraph(size_t max_nodes)
      : adjacency_list(AdjacencyList{max_nodes}) {}
  // first entry of pair is vertex numeral, second entry is color
  std::unordered_map<size_t, size_t>
  color(std::vector<std::pair<size_t, size_t>> precolored_nodes,
        size_t num_nodes);
  void add_edge(const size_t a, const size_t b) {
    adjacency_list.add_edge(a, b);
  }
  void add_clique(std::unordered_set<size_t> &clique) {
    adjacency_list.add_clique_optimized(clique);
  }
  [[nodiscard]] const AdjacencyList &get_adjacent_list() const {
    return adjacency_list;
  }
};

#endif // COMPILER_GRAPH_COLORING_H
