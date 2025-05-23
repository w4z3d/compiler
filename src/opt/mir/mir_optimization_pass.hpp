#ifndef OPT_MIR_MIR_OPTIMIZATION_PASS_H
#define OPT_MIR_MIR_OPTIMIZATION_PASS_H

#include "../../mir/mir.hpp"
#include <iostream>
#include <string>
#include <utility>

class MIROptPass {

private:
  std::string name;

  virtual void transform_function(mir::MachineFunction &function) = 0;

public:
  virtual ~MIROptPass() = default;
  explicit MIROptPass(std::string name) : name(std::move(name)) {}

  inline void perform_pass(mir::MIRProgram &program) {
    for (auto &fun : program.get_functions()) {
      transform_function(fun.second);
    }
  }

  [[nodiscard]] const std::string &get_name() const { return name; }
};

struct MIROptPhase {
  std::list<MIROptPass *> passes{};
  explicit MIROptPhase(std::list<MIROptPass *> passes)
      : passes(std::move(passes)) {}

  void perform_passes(mir::MIRProgram &program) {
    for (const auto &pass : passes) {
      std::cout << "Performing " << pass->get_name() << std::endl;
      pass->perform_pass(program);
    }
  }

  ~MIROptPhase() {
    for (auto *pass : passes) {
      delete pass;
    }
  }
};

#endif // !COMPILER_MIR_HOPTIMIZATION_PASS_H
