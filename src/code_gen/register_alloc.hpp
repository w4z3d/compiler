#ifndef COMPILER_REGISTER_ALLOC_H
#define COMPILER_REGISTER_ALLOC_H

#include "interference_graph.hpp"
class RegisterAllocation {
private:
  InterferenceGraph &ig;
  std::unordered_map<Var, size_t> var_to_color{};
  std::vector<Var> soe{}; // Simplicial Elimination Ordering
  static Var get_max_key(std::unordered_set<Var> &set,
                         std::unordered_map<Var, size_t> &map);
  void maximum_cardinality_search();
  size_t get_color(Var v);
  void greedy_coloring();

public:
  explicit RegisterAllocation(InterferenceGraph &ig) : ig(ig) {}
  void color();

  [[nodiscard]] const std::unordered_map<Var, size_t> &get_result() const {
    return var_to_color;
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
      oss << "  \"" << from.to_string() << "\""
          << std::format("[label=\"{}\"]", var_to_color.find(from)->second)
          << ";\n";
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
