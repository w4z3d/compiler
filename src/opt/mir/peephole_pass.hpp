#ifndef OPT_MIR_PEEPHOLE_PASS_H
#define OPT_MIR_PEEPHOLE_PASS_H
#include "../../mir/mir.hpp"
#include "mir_optimization_pass.hpp"
#include <cstdint>
#include <functional>
#include <vector>

using PeepholePatternFunction = std::function<bool(
    mir::MachineBasicBlock *block,
    std::list<mir::MachineInstruction *>::iterator &current_iter)>;

class MIRPeepholePass : public MIROptPass {
private:
  std::uint8_t window_size;
  std::vector<PeepholePatternFunction> patterns{};

  void transform_block(mir::MachineBasicBlock *block) override;

  void transform_function(mir::MachineFunction &function) {
    transform_block(function.get_entry_block());
  }

  // Optimizations
  static bool optimize_redundant_mov_rr(
      mir::MachineBasicBlock *block,
      std::list<mir::MachineInstruction *>::iterator &inst_iter);
  static bool optimize_mul_by_power_of_two(
      mir::MachineBasicBlock *block,
      std::list<mir::MachineInstruction *>::iterator &inst_iter);

public:
  explicit MIRPeepholePass(std::uint8_t window_size = 1)
      : MIROptPass("Peephole"), window_size(window_size) {
    patterns.emplace_back(MIRPeepholePass::optimize_redundant_mov_rr);
  }

  void run(mir::MIRProgram &program) {
    for (auto &fun : program.get_functions()) {
      transform_function(fun.second);
    }
  }
};

#endif // !OPT_MIR_PEEPHOLE_PASS_H
