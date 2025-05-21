#ifndef OPT_MIR_PEEPHOLE_PASS_H
#define OPT_MIR_PEEPHOLE_PASS_H
#include "../../mir/mir.hpp"
#include "mir_optimization_pass.hpp"
class MIRPeepholePass : public MIROptPass {
private:
  void transform_block(mir::MachineBasicBlock *block) override;

  void transform_function(mir::MachineFunction &function) {
    transform_block(function.get_entry_block());
  }

public:
  MIRPeepholePass() : MIROptPass("Peephole") {}

  void run(mir::MIRProgram &program) {
    for (auto &fun : program.get_functions()) {
      transform_function(fun.second);
    }
  }
};

#endif // !OPT_MIR_PEEPHOLE_PASS_H
