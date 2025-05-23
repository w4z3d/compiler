#ifndef OPT_MIR_PEEPHOLE_PASS_H
#define OPT_MIR_PEEPHOLE_PASS_H
#include "../../mir/mir.hpp"
#include "mir_optimization_pass.hpp"
#include <cstdint>
#include <functional>
#include <vector>

class MIRPeepholePass : public MIROptPass {
private:
  std::uint8_t window_size;

  void transform_function(mir::MachineFunction &function) override;

  // Optimizations
  static bool optimize_redundant_mov_rr(
      mir::MachineFunction &block,
      std::list<mir::MachineInstruction *>::iterator &inst_iter);
  static bool optimize_mul_by_power_of_two(
      mir::MachineFunction &block,
      std::list<mir::MachineInstruction *>::iterator &inst_iter);

public:
  explicit MIRPeepholePass(std::uint8_t window_size = 1)
      : MIROptPass("Peephole"), window_size(window_size) {}

  void run(mir::MIRProgram &program) {
    for (auto &fun : program.get_functions()) {
      transform_function(fun.second);
    }
  }
};

#endif // !OPT_MIR_PEEPHOLE_PASS_H
