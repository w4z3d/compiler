#ifndef COMPILER_REGISTER_ALLOC_H
#define COMPILER_REGISTER_ALLOC_H

#include "../analysis/liveness.hpp"
#include "interference_graph.hpp"
#include "target/target.hpp"

class RegisterAllocation {
private:
  arena::Arena arena;
  InterferenceGraph ig;
  Liveness &liveness;
  MIRRegisterMap &rmap;
  mir::MachineFunction &function;
  const Target &target;

public:
  explicit RegisterAllocation(Liveness &liveness, MIRRegisterMap &rmap,
                              mir::MachineFunction &function,
                              const Target &target)
      : liveness(liveness), rmap(rmap), function(function),
        ig(InterferenceGraph{liveness.get_liveness(), rmap, function}),
        target(target), arena(arena::Arena{}) {}

  // graph coloring, color to register/stack slot, also change Regs directly in
  // mir::MachineFunction?
  void allocate() {
    size_t slot_counter = 1;
    const auto gprs = target.get_gprs();
    std::list<mir::PhysicalRegister> unused_regs{gprs.begin(), gprs.end()};
    std::unordered_map<size_t, mir::PhysicalRegister> color_to_physical_reg{};
    std::unordered_map<size_t, mir::StackSlot> color_to_stack_slot{};
    std::cout << "constructing ig" << std::endl;
    ig.construct();
    std::cout << "done" << std::endl;
    std::cout << "coloring" << std::endl;
    std::unordered_map<size_t, size_t> color_map = std::move(ig.color());
    std::cout << "done" << std::endl;
    // get mapping of used physical registers
    for (const auto &live_id : rmap.get_physical_live_ids()) {
      const mir::PhysicalRegister reg = get_physical_reg_from_string(
          rmap.physical_from_live(live_id).value());
      color_to_physical_reg.emplace(color_map[live_id], reg);
      unused_regs.remove(reg);
    }

    for (const auto &[live_id, color] : color_map) {
      if (unused_regs.size() == 1) {
        break;
      }
      color_to_physical_reg.emplace(color, unused_regs.front());
      unused_regs.pop_front();
    }

    const auto spilling_reg = unused_regs.front();

    for (const auto &[live_id, color] : color_map) {
      if (color_to_physical_reg.contains(color))
        continue;
      color_to_stack_slot.emplace(color, mir::StackSlot{slot_counter++ * 4});
    }

    std::cout << "replacing virtual regs with physical/stack slot" << std::endl;
    for (auto inst = function.get_entry_block()->get_instructions().begin();
         inst != function.get_entry_block()->get_instructions().end(); ++inst) {
      int i = 0;
      for (auto &item : (*inst)->get_ins_mut()) {
        i++;
        if (std::holds_alternative<mir::VirtualRegister>(item.get_op())) {
          auto reg = std::get<mir::VirtualRegister>(item.get_op());
          auto color = color_map[rmap.from_virtual(reg.get_numeral())];
          if (color_to_physical_reg.contains(color)) {
            item.replace_with_physical(color_to_physical_reg.at(color));
          } else {
            if (i == 1 && (*inst)->get_ins().size() > 1) {
              const auto slot_move = arena.create<mir::MachineInstruction>(
                  mir::MachineInstruction::MachineOpcode::LOAD_REG_MEM,
                  std::vector<mir::MachineOperand>{
                      mir::MachineOperand{color_to_stack_slot.at(color)}},
                  std::vector<mir::MachineOperand>{
                      mir::MachineOperand{spilling_reg}});
              function.get_entry_block()->get_instructions().insert(inst,
                                                                    slot_move);
              item.replace_with_physical(spilling_reg);
            } else {
              if ((*inst)->get_opcode() == mir::MachineInstruction::MachineOpcode::MOV_RR) {
                (*inst)->set_opcode(mir::MachineInstruction::MachineOpcode::LOAD_REG_MEM);
              }
              item.replace_with_stack_slot(color_to_stack_slot.at(color));
            }
          }
        }
      }
      for (auto &item : (*inst)->get_outs_mut()) {
        if (std::holds_alternative<mir::VirtualRegister>(item.get_op())) {
          auto reg = std::get<mir::VirtualRegister>(item.get_op());
          auto color = color_map[rmap.from_virtual(reg.get_numeral())];
          if (color_to_physical_reg.contains(color)) {
            item.replace_with_physical(color_to_physical_reg.at(color));
          } else {
            const auto slot_move = arena.create<mir::MachineInstruction>(
                mir::MachineInstruction::MachineOpcode::STORE_MEM_REG,
                std::vector<mir::MachineOperand>{
                    mir::MachineOperand{spilling_reg}},
                std::vector<mir::MachineOperand>{
                    mir::MachineOperand{color_to_stack_slot.at(color)}});
            function.get_entry_block()->get_instructions().insert(std::next(inst),
                                                                  slot_move);
            item.replace_with_physical(spilling_reg);
          }
        }
      }
    }

    function.set_frame_size(color_to_stack_slot.size() * 4);
    std::cout << "done" << std::endl;
  }

  mir::PhysicalRegister get_physical_reg_from_string(const std::string &name) {
    for (const auto &item : target.get_gprs()) {
      if (item.get_name() == name) {
        return item;
      }
    }
    throw std::runtime_error("gedagedigedagedago");
  }
};

#endif // COMPILER_REGISTER_ALLOC_H
