#include "interference_graph.hpp"

void InterferenceGraph::construct() {
  for (const auto &[liveness, instruction] :
       std::views::zip(block_to_live.begin()->second,
                       function.get_entry_block()->get_instructions())) {
    graph.add_clique(liveness);
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
  }
}
std::unordered_map<size_t, size_t> InterferenceGraph::color() { return std::move(graph.color({})); }
