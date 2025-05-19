#ifndef COMPILER_GRAPH_COLORING_H
#define COMPILER_GRAPH_COLORING_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

struct UndirectedGraph { // but is it really chordal :(
private:
  std::unordered_map<size_t, std::unordered_set<size_t>> adjacent_map{};

  static size_t get_max_key(std::unordered_set<size_t> &set,
                            std::unordered_map<size_t, size_t> &map);
  std::vector<size_t> maximum_cardinality_search(
      std::vector<size_t> &seo,
      std::vector<std::pair<size_t, size_t>>
          &precolored_nodes); // returns simplicial elimination ordering
  size_t get_color(size_t v, std::unordered_map<size_t, size_t> &var_to_color);
  void greedy_coloring(std::vector<size_t> &soe,
                       std::unordered_map<size_t, size_t> &var_to_color);

public:
  // first entry of pair is vertex numeral, second entry is color
  std::unordered_map<size_t, size_t>
  color(std::vector<std::pair<size_t, size_t>> precolored_nodes);
  void add_edge(const size_t a, const size_t b) {
    if (a == b)
      return;
    if (adjacent_map.contains(a)) {
      adjacent_map[a].insert(b);
    } else {
      adjacent_map[a] = std::unordered_set<size_t>{b};
    }
    if (adjacent_map.contains(b)) {
      adjacent_map[b].insert(a);
    } else {
      adjacent_map[b] = std::unordered_set<size_t>{a};
    }
  }
  void add_clique(std::unordered_set<size_t> &clique) {
    for (const auto &first : clique) {
      for (const auto &second : clique) {
        add_edge(first, second);
      }
    }
  }
  [[nodiscard]] const std::unordered_map<size_t, std::unordered_set<size_t>> &get_adjacent_list() const {
    return adjacent_map;
  }
};

#endif // COMPILER_GRAPH_COLORING_H
