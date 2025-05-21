#ifndef COMPILER_INTERFERENCE_GRAPH_H
#define COMPILER_INTERFERENCE_GRAPH_H

#include "../graph_coloring/graph_coloring.hpp"
#include "../ir/ir.hpp"
#include "../mir/mir.hpp"
#include "yapper.hpp"
#include <ranges>

template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};

class InterferenceGraph {
private:
  UndirectedGraph graph;
  std::unordered_map<size_t, std::list<std::unordered_set<size_t>>>
      &block_to_live;
  MIRRegisterMap &rmap;
  mir::MachineFunction &function;

public:
  explicit InterferenceGraph(
      std::unordered_map<size_t, std::list<std::unordered_set<size_t>>> &live,
      MIRRegisterMap &rmap, mir::MachineFunction &function)
      : block_to_live(live), rmap(rmap), function(function),
        graph(UndirectedGraph{rmap.get_size() + 16}) {}// TODO: not hardcoded
  void construct();
  std::unordered_map<size_t, size_t> color();

  std::string to_string() {
    std::ostringstream oss;
    for (const auto &[key, neighbors] : graph.get_adjacent_list().smart()) {
      if (rmap.physical_from_live(key).has_value()) {
        oss << rmap.physical_from_live(key).value() << " -> {";
      } else if (rmap.virtual_from_live(key).has_value()) {
        oss << rmap.virtual_from_live(key).value() << " -> {";
      } else {
        oss << "no var mapped with id " << key << std::endl;
        continue;
      }

      bool first = true;
      for (const auto &neighbor : neighbors) {
        if (!first)
          oss << ", ";
        if (rmap.physical_from_live(neighbor).has_value()) {
          oss << rmap.physical_from_live(neighbor).value();
        } else if (rmap.virtual_from_live(neighbor).has_value()) {
          oss << rmap.virtual_from_live(neighbor).value();
        } else {
          oss << "invalid=(" << neighbor << ")";
        }
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

    for (const auto &[from, neighbors] : graph.get_adjacent_list().smart()) {
      if (rmap.physical_from_live(from).has_value()) {
        oss << "  \"" << rmap.physical_from_live(from).value() << "\";\n";
      } else {
        oss << "  \"" << rmap.virtual_from_live(from).value() << "\";\n";
      }
      for (const auto &to : neighbors) {
        if (rmap.physical_from_live(from).has_value()) {
          oss << "  \"" << rmap.physical_from_live(from).value() << "\" "
              << edge_op << " \"";
        } else {
          oss << "  \"" << rmap.virtual_from_live(from).value() << "\" "
              << edge_op << " \"";
        }
        if (rmap.physical_from_live(to).has_value()) {
          oss << rmap.physical_from_live(to).value() << "\";\n";
        } else {
          oss << rmap.virtual_from_live(to).value() << "\";\n";
        }
      }
    }

    oss << "}\n";
    return oss.str();
  }
};

#endif // COMPILER_INTERFERENCE_GRAPH_H
