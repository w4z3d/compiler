#include "graph_coloring.hpp"

size_t UndirectedGraph::get_max_key(std::unordered_set<size_t> &set,
                                    std::unordered_map<size_t, size_t> &map) {
  size_t max_v = 0;
  size_t max_k{0};
  for (const auto &item : set) {
    const auto value = map.find(item)->second;
    if (value >= max_v) {
      max_k = item;
      max_v = value;
    }
  }
  return max_k;
}
std::vector<size_t> UndirectedGraph::maximum_cardinality_search(
    std::vector<size_t> &seo,
    std::vector<std::pair<size_t, size_t>> &precolored_nodes) {
  std::unordered_map<size_t, size_t> weight{};
  std::unordered_set<size_t> V{};
  std::unordered_set<size_t> W{};
  for (const auto &item : adjacent_map) {
    weight.emplace(item.first, 0);
    V.insert(item.first);
    W.insert(item.first);
  }
  // remove precolored nodes from selection, but still increase weight function
  // (Source: Vorlesung GRRR)
  for (const auto &v : precolored_nodes) {
    std::unordered_set<size_t> &neighbours_v = adjacent_map[v.first];
    for (const auto &item : W) {
      if (neighbours_v.contains(item)) {
        weight[item] = weight[item] + 1;
      }
    }
    W.erase(v.first);
    V.erase(v.first);
  }
  // Build SEO
  for (int i = 0; i < V.size(); i++) {
    size_t v = get_max_key(W, weight);
    seo.push_back(v);
    std::unordered_set<size_t> &neighbours_v = adjacent_map[v];
    for (const auto &item : W) {
      if (neighbours_v.contains(item)) {
        weight[item] = weight[item] + 1;
      }
    }
    W.erase(v);
  }
  return seo;
}

size_t
UndirectedGraph::get_color(size_t v,
                           std::unordered_map<size_t, size_t> &var_to_color) {
  size_t color = 0;
  std::unordered_set<size_t> &neighbours_v = adjacent_map[v];
  std::unordered_set<size_t> neighbour_colors{};
  for (const auto &item : neighbours_v) {
    if (var_to_color.contains(item)) {
      neighbour_colors.insert(var_to_color.find(item)->second);
    }
  }
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
    std::vector<std::pair<size_t, size_t>> precolored_nodes) {
  std::vector<size_t> soe{};
  std::unordered_map<size_t, size_t> var_to_color{};

  // calculate SEO
  maximum_cardinality_search(soe, precolored_nodes);
  // set pre colors
  for (const auto &item : precolored_nodes) {
    var_to_color[item.first] = item.second;
  }
  greedy_coloring(soe, var_to_color);
  return var_to_color;
}
