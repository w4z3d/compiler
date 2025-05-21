#ifndef OPT_MIR_MIR_OPTIMIZATION_PASS_H
#define OPT_MIR_MIR_OPTIMIZATION_PASS_H

#include "../../ir/cfg.hpp"
#include <iostream>
#include <list>
#include <string>
#include <utility>

class IROptPass {

private:
  std::string name;
  virtual void transform_block(BasicBlock *block) = 0;

public:
  virtual ~IROptPass() = default;
  explicit IROptPass(std::string name) : name(std::move(name)) {}

  inline void perform_pass(IntermediateRepresentation &program) {
    for (auto &fun : program.get_cfgs()) {
      transform_block(fun.get_entry_block_mut());
    }
  }

  [[nodiscard]] const std::string &get_name() const { return name; }
};

struct IROptPhase {
  std::list<IROptPass *> passes{};
  explicit IROptPhase(std::list<IROptPass *> passes)
      : passes(std::move(passes)) {}

  void perform_passes(IntermediateRepresentation &program) {
    for (const auto &pass : passes) {
      std::cout << "Performing " << pass->get_name() << std::endl;
      pass->perform_pass(program);
    }
  }

  ~IROptPhase() {
    for (auto *pass : passes) {
      delete pass;
    }
  }
};

#endif // !COMPILER_MIR_HOPTIMIZATION_PASS_H
