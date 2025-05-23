#include "liveness.hpp"

#include <ranges>

template <typename T>
std::unordered_set<T> difference(std::unordered_set<T> a, T b) {
  std::unordered_set<T> result;
  a.erase(b);
  return a;
}

template <typename T>
void union_sets(std::unordered_set<T> &a, const std::unordered_set<T> &b) {
  for (const auto &elem : b) {
    a.insert(elem);
  }
}

void Liveness::analyse() {
  for (const auto &item : mir_program.get_functions()) {
    analyse_func(item.second);
  }
}

template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};

void Liveness::analyse_func(const mir::MachineFunction &machine_function) {
  // TODO: maybe clear some stuff here? Relevant for multiple functions.
  // TODO: changed flag, to check when to stop. For now one pass is enough.
  // TODO: while(not changed) do:
  std::list<std::unordered_set<size_t>> lives_per_line{};
  std::unordered_set<size_t> prev_line{};
  for (const auto &instr :
       std::ranges::reverse_view(machine_function.get_instructions())) {
    lives_per_line.emplace_front();
    for (const auto &in_operand : instr->get_ins()) {
      // oh
      std::visit(
          overload{[this, &lives_per_line](const mir::VirtualRegister &r) {
                     lives_per_line.front().insert(
                         rmap.from_virtual(r.get_numeral()));
                   },
                   [this, &lives_per_line](const mir::PhysicalRegister &r) {
                     lives_per_line.front().insert(
                         rmap.from_physical(r.get_name()));
                   },
                   [](auto &) {}},
          in_operand.get_op());
    }
    // prev line - definitions
    for (const auto &out_operand : instr->get_outs()) {
      // oh
      std::visit(overload{[this, &prev_line](const mir::VirtualRegister &r) {
                            prev_line.erase(rmap.from_virtual(r.get_numeral()));
                          },
                          [this, &prev_line](const mir::PhysicalRegister &r) {
                            prev_line.erase(rmap.from_physical(r.get_name()));
                          },
                          [](auto &) {}},
                 out_operand.get_op());
    }
    // union reduced previous line with this line
    union_sets(lives_per_line.front(), prev_line);
    // set current line to prev line
    prev_line = lives_per_line.front();
  }
  block_to_live.emplace(machine_function.get_id(), lives_per_line);
}
