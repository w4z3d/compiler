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
    analyse_func(item);
  }
}

template <class... Ts> struct overload : Ts... {
  using Ts::operator()...;
};

// Calculate basic block order for liveness analysis passes & add var mappings
void Liveness::dfs_basic_block(mir::MachineBasicBlock *bb,
                               std::unordered_set<size_t> &visited) {
  if (visited.contains(bb->get_id()))
    return;
  visited.insert(bb->get_id());
  for (auto item : bb->get_successors()) {
    dfs_basic_block(item, visited);
  }
  basic_block_order.push_back(bb);
  for (const auto instr : bb->get_instructions()) {
    for (const auto &in_operand : instr->get_ins()) {
      // oh
      std::visit(overload{[this](const mir::VirtualRegister &r) {
                            if (virtual_to_live_id.contains(r.get_numeral()))
                              return;
                            std::size_t id = id_counter++;
                            virtual_to_live_id[r.get_numeral()] = id;
                            live_id_to_virtual[id] = r.get_numeral();
                          },
                          [this](const mir::PhysicalRegister &r) {
                            if (physical_to_live_id.contains(r.get_name()))
                              return;
                            std::size_t id = id_counter++;
                            physical_to_live_id[r.get_name()] = id;
                            live_id_to_physical[id] = r.get_name();
                          },
                          [](auto &) {}},
                 in_operand.get_op());
    }
    for (const auto &out_operand : instr->get_outs()) {
      // oh
      std::visit(overload{[this](const mir::VirtualRegister &r) {
                            if (virtual_to_live_id.contains(r.get_numeral()))
                              return;
                            std::size_t id = id_counter++;
                            virtual_to_live_id[r.get_numeral()] = id;
                            live_id_to_virtual[id] = r.get_numeral();
                          },
                          [this](const mir::PhysicalRegister &r) {
                            if (physical_to_live_id.contains(r.get_name()))
                              return;
                            std::size_t id = id_counter++;
                            physical_to_live_id[r.get_name()] = id;
                            live_id_to_physical[id] = r.get_name();
                          },
                          [](auto &) {}},
                 out_operand.get_op());
    }
  }
}

void Liveness::analyse_func(const mir::MachineFunction &machine_function) {
  // TODO: maybe clear some stuff here? Relevant for multiple functions.
  std::unordered_set<size_t> visited{};
  dfs_basic_block(machine_function.get_entry_block(), visited);
  std::cout << "bb order: ";
  for (const auto &item : basic_block_order) {
    std::cout << item->get_id() << " ";
  }
  std::cout << std::endl;
  // TODO: changed flag, to check when to stop. For now one pass is enough.
  // TODO: while(not changed) do:
  std::list<std::unordered_set<size_t>> lives_per_line{};
  std::unordered_set<size_t> prev_line{};
  for (const auto bb : basic_block_order) {
    for (const auto &instr :
         std::ranges::reverse_view(bb->get_instructions())) {
      lives_per_line.emplace_front();
      for (const auto &in_operand : instr->get_ins()) {
        // oh
        std::visit(overload{[this, &lives_per_line](const mir::VirtualRegister &r) {
                              lives_per_line.front().insert(virtual_to_live_id[r.get_numeral()]);
                            },
                            [this, &lives_per_line](const mir::PhysicalRegister &r) {
                              lives_per_line.front().insert(physical_to_live_id[r.get_name()]);
                            },
                            [](auto &) {}},
                   in_operand.get_op());
      }
      // prev line - definitions
      for (const auto &out_operand : instr->get_outs()) {
        // oh
        std::visit(overload{[this, &prev_line](const mir::VirtualRegister &r) {
                              prev_line.erase(virtual_to_live_id[r.get_numeral()]);
                            },
                            [this, &prev_line](const mir::PhysicalRegister &r) {
                              prev_line.erase(physical_to_live_id[r.get_name()]);
                            },
                            [](auto &) {}},
                   out_operand.get_op());
      }
      // union reduced previous line with this line
      union_sets(lives_per_line.front(), prev_line);
      // set current line to prev line
      prev_line = lives_per_line.front();
    }
    block_to_live.emplace(bb->get_id(), lives_per_line);
  }
}
