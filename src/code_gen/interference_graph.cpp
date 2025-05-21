#include "interference_graph.hpp"
#include <chrono>

void InterferenceGraph::construct() {
  //long long clique = 0;
  //long long impl = 0;
  //auto s0 = std::chrono::high_resolution_clock::now();
  for (const auto &[liveness, instruction] :
       std::views::zip(block_to_live.begin()->second,
                       function.get_entry_block()->get_instructions())) {
    //auto s1 = std::chrono::high_resolution_clock::now();
    graph.add_clique(liveness);
    //auto s2 = std::chrono::high_resolution_clock::now();
    for (const auto &operand : instruction->get_implicit_defs()) {
      std::visit(overload{[this, &liveness](const mir::PhysicalRegister &r) {
                            size_t reg_id = rmap.from_physical(r.get_name());
                            for (const auto &item : liveness) {
                              graph.add_edge(reg_id, item);
                            }
                          },
                          [this, &liveness](const mir::VirtualRegister &r) {
                            size_t reg_id = rmap.from_virtual(r.get_numeral());
                            for (const auto &item : liveness) {
                              graph.add_edge(reg_id, item);
                            }
                          },
                          [](auto &) {}},
                 operand.get_op());
    }
    //auto s3 = std::chrono::high_resolution_clock::now();
    //clique += duration_cast<std::chrono::milliseconds>(s2 - s1).count();
    //impl += duration_cast<std::chrono::milliseconds>(s3 - s2).count();
  }
  //auto s4 = std::chrono::high_resolution_clock::now();
  //auto x = duration_cast<std::chrono::milliseconds>(s4 - s0).count();
  //std::cout << "Clique: \t" << clique/1000. << std::endl;
  //std::cout << "Impl: \t\t" << impl/1000. << std::endl;
  //std::cout << "All: \t\t" << x/1000. << std::endl;
  //std::cout << "Util: \t\t" << (x - impl - clique)/1000. << std::endl;
}
std::unordered_map<size_t, size_t> InterferenceGraph::color() { return std::move(graph.color({}, rmap.get_size())); }
