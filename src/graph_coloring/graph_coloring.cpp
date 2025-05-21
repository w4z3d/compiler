#include "graph_coloring.hpp"

size_t UndirectedGraph::get_max_key(BitSet &set, std::vector<size_t> &weight) {
  size_t max_v = 0;
  size_t max_k{0};
  for (int i = 0; i < set.size(); i++) {
    if (set.test(i)) {
      const auto value = weight[i];
      if (value >= max_v) {
        max_k = i;
        max_v = value;
      }
    }
  }
  /*for (const auto &item : set) {
    const auto value = weight[item];
    if (value >= max_v) {
      max_k = item;
      max_v = value;
    }
  }*/
  return max_k;
}
std::vector<size_t> UndirectedGraph::maximum_cardinality_search(
    std::vector<size_t> &seo,
    const std::vector<std::pair<size_t, size_t>> &precolored_nodes,
    size_t num_nodes) {
  std::vector<size_t> weight(num_nodes, 0);
  BitSet V{num_nodes, true};
  BitSet W{num_nodes, true};
  // remove precolored nodes from selection, but still increase weight function
  // (Source: Vorlesung GRRR)
  for (const auto &v : precolored_nodes) {
    BitSet &neighbours_v = adjacency_list.neighbors(v.first);
    for (int i = 0; i < W.size(); i++) {
      if (W.test(i)) {
        if (neighbours_v.test(i)) {
          weight[i]++;
        }
      }
    }
    /*for (const auto &item : W) {
      if (neighbours_v.test(item)) {
        weight[item]++;
      }
    }*/
    W.reset(v.first);
    V.reset(v.first);
  }
  // Build SEO
  for (int i = 0; i < V.count(); i++) {
    size_t v = get_max_key(W, weight);
    seo.push_back(v);
    BitSet &neighbours_v = adjacency_list.neighbors(v);
    for (int n = 0; i < W.size(); i++) {
      if (W.test(n)) {
        if (neighbours_v.test(n)) {
          weight[n]++;
        }
      }
    }
    /*for (const auto &item : W) {
      if (neighbours_v.test(item)) {
        weight[item]++;
      }
    }*/
    W.reset(v);
  }
  return seo;
}

size_t
UndirectedGraph::get_color(size_t v,
                           std::unordered_map<size_t, size_t> &var_to_color) {
  size_t color = 0;
  BitSet &neighbours_v = adjacency_list.neighbors(v);
  std::unordered_set<size_t> neighbour_colors{};
  for (int i = 0; i < neighbours_v.size(); i++) {
    if (neighbours_v.test(i)) {
      if (var_to_color.contains(i)) {
        neighbour_colors.insert(var_to_color.find(i)->second);
      }
    }
  }
  /*for (const auto &item : neighbours_v) {
    if (var_to_color.contains(item)) {
      neighbour_colors.insert(var_to_color.find(item)->second);
    }
  }*/
  while (neighbour_colors.contains(color)) {
    color++;
  }
  return color;
}

void UndirectedGraph::greedy_coloring(
    std::vector<size_t> &soe,
    std::unordered_map<size_t, size_t> &var_to_color) {
  for (const auto &item : soe) {
    var_to_color[item] = get_color(item, var_to_color);
  }
}

std::unordered_map<size_t, size_t> UndirectedGraph::color(
    const std::vector<std::pair<size_t, size_t>> precolored_nodes,
    size_t num_nodes) {
  std::cout << num_nodes << std::endl;
  std::vector<size_t> soe{};
  std::unordered_map<size_t, size_t> var_to_color{};

  // calculate SEO
  maximum_cardinality_search(soe, precolored_nodes, num_nodes);
  // set pre colors
  for (const auto &item : precolored_nodes) {
    var_to_color[item.first] = item.second;
  }

  greedy_coloring(soe, var_to_color);
  return var_to_color;
}
