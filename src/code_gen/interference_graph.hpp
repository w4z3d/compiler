#ifndef COMPILER_INTERFERENCE_GRAPH_H
#define COMPILER_INTERFERENCE_GRAPH_H

#include "../ir/ir.hpp"

class InterferenceGraph {
private:
  std::unordered_map<Var, std::unordered_set<Var>> adjacent_map{};
  std::unordered_map<size_t, std::vector<std::unordered_set<Var>>>
      &block_to_live;

public:
  explicit InterferenceGraph(
      std::unordered_map<size_t, std::vector<std::unordered_set<Var>>> &live)
      : block_to_live(live) {}
  std::unordered_map<Var, std::unordered_set<Var>> &get_adjacent_map() {
    return adjacent_map;
  }
  void construct();

  std::string to_string() {
    std::ostringstream oss;
    for (const auto &[key, neighbors] : adjacent_map) {
      oss << key.to_string() << " -> {";
      bool first = true;
      for (const auto &neighbor : neighbors) {
        if (!first)
          oss << ", ";
        oss << neighbor.to_string();
        first = false;
      }
      oss << "}\n";
    }
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

    for (const auto &[from, neighbors] : adjacent_map) {
      oss << "  \"" << from.to_string() << "\";\n";
      for (const auto &to : neighbors) {
        oss << "  \"" << from.to_string() << "\" " << edge_op << " \""
            << to.to_string() << "\";\n";
      }
    }

    oss << "}\n";
    return oss.str();
  }
};

#endif // COMPILER_INTERFERENCE_GRAPH_H
