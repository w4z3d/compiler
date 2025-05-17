#include "register_alloc.hpp"
Var RegisterAllocation::get_max_key(std::unordered_set<Var> &set,
                                    std::unordered_map<Var, size_t> &map) {
  size_t max_v = 0;
  Var max_k{0};
  for (const auto &item : set) {
    const auto value = map.find(item)->second;
    if (value >= max_v) {
      max_k = item;
      max_v = value;
    }
  }
  return max_k;
}
void RegisterAllocation::maximum_cardinality_search() {
  std::unordered_map<Var, size_t> weight{};
  std::unordered_set<Var> V{};
  std::unordered_set<Var> W{};
  for (const auto &item : ig.get_adjacent_map()) {
    weight.emplace(item.first.numeral, 0);
    V.insert(item.first);
    W.insert(item.first);
  }
  for (int i = 0; i < V.size(); i++) {
    Var v = get_max_key(W, weight);
    soe.push_back(v);
    std::unordered_set<Var> &neighbours_v = ig.get_adjacent_map()[v];
    for (const auto &item : W) {
      if (neighbours_v.contains(item)) {
        weight[item] = weight[item] + 1;
      }
    }
    W.erase(v);
  }
}

size_t RegisterAllocation::get_color(Var v) {
  size_t color = 0;
  std::unordered_set<Var> &neighbours_v = ig.get_adjacent_map()[v];
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

void RegisterAllocation::greedy_coloring() {
  for (const auto &item : soe) {
    var_to_color[item] = get_color(item);
  }
}

void RegisterAllocation::color() {
  maximum_cardinality_search();
  greedy_coloring();
}
