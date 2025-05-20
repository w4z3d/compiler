#ifndef OPT_MIR_MIR_OPTIMIZATION_PASS_H
#define OPT_MIR_MIR_OPTIMIZATION_PASS_H

#include "../../mir/mir.hpp"
#include <iostream>
#include <string>
#include <utility>
class MIROptPass {
  virtual void transform_block(mir::MachineBasicBlock *block) = 0;

public:
  std::string name;
  explicit MIROptPass(std::string name) : name(std::move(name)) {}
};

#endif // !COMPILER_MIR_HOPTIMIZATION_PASS_H
