#ifndef COMPILER_REGISTER_ALLOC_H
#define COMPILER_REGISTER_ALLOC_H

#include "interference_graph.hpp"
class RegisterAllocation {
private:
  InterferenceGraph &ig;
  std::unordered_map<Var, size_t> var_to_color{};
  std::vector<Var> soe{}; // Simplicial Elimination Ordering
  static Var get_max_key(std::unordered_set<Var> &set,
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
  void maximum_cardinality_search() {
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
  size_t get_color(Var v) {
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
  void greedy_coloring() {
    for (const auto &item : soe) {
      var_to_color[item] = get_color(item);
    }
  }

public:
  explicit RegisterAllocation(InterferenceGraph &ig) : ig(ig) {}
  void color() {
    maximum_cardinality_search();
    greedy_coloring();
  }
  [[nodiscard]] std::string soe_to_string() const {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < soe.size(); ++i) {
      oss << soe[i].to_string();
      if (i + 1 < soe.size())
        oss << ", ";
    }
    oss << "]";
    return oss.str();
  }
  std::string to_dot(const std::string &graph_name = "G",
                     bool directed = false) {
    std::ostringstream oss;
    std::string edge_op = directed ? "->" : "--";
    std::string graph_type = directed ? "digraph" : "graph";

    oss << graph_type << " " << graph_name << " {\n";

    std::unordered_set<std::pair<Var, Var>,
                       std::function<std::size_t(const std::pair<Var, Var> &)>>
        printed_edges(10, [](const std::pair<Var, Var> &p) {
          return std::hash<Var>()(p.first) ^ (std::hash<Var>()(p.second) << 1);
        });

    for (const auto &[from, neighbors] : ig.get_adjacent_map()) {
      oss << "  \"" << from.to_string() << "\"" << std::format("[label=\"{}\"]", var_to_color.find(from)->second) << ";\n";
      for (const auto &to : neighbors) {
        oss << "  \"" << from.to_string() << "\" " << edge_op << " \""
            << to.to_string() << "\";\n";
      }
    }

    oss << "}\n";
    return oss.str();
  }
};

#endif // COMPILER_REGISTER_ALLOC_H
