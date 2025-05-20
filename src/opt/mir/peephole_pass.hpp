#ifndef OPT_MIR_PEEPHOLE_PASS_H
#define OPT_MIR_PEEPHOLE_PASS_H
#include "../../mir/mir.hpp"
#include <iostream>
class MIRPeepholePass {
private:
  void transform_block(mir::MachineBasicBlock *block);

  void transform_function(mir::MachineFunction &function) {
    transform_block(function.get_entry_block());
  }

public:
  void run(mir::MIRProgram &program) {
    for (auto &fun : program.get_functions()) {
      transform_function(fun.second);
    }
  }
};

#endif // !OPT_MIR_PEEPHOLE_PASS_H
