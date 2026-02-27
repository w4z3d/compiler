#ifndef SRC_HIR_OPT_OPTIMIZATION_PIPELINE_HPP
#define SRC_HIR_OPT_OPTIMIZATION_PIPELINE_HPP
#include "../hir.hpp"
bool constant_folding(hir::Function &fn) {
  bool changed = false;
  for (auto *bb : fn.blocks) {
    // Use iterator since we might erase instructions
    auto it = bb->instructions.begin();
    while (it != bb->instructions.end()) {
      auto *inst = *it;

      if (inst->opcode == hir::Opcode::Add) {
        auto *lhs = dynamic_cast<hir::ConstantInt *>(inst->operand(0));
        auto *rhs = dynamic_cast<hir::ConstantInt *>(inst->operand(1));
        if (lhs && rhs) {
          auto *result = fn.parent->const_int(
              dynamic_cast<hir::type::IntegerType *>(inst->type),
              lhs->signed_value() + rhs->signed_value());
          inst->replace_all_uses_with(result);
          it = bb->instructions.erase(it);
          changed = true;
          continue;
        }
      }
      ++it;
    }
  }
  return changed;
}
struct OptPipeline {
  static inline void optimize(hir::Module &module) {
    for (auto &fn : module.functions) {
      if (fn->is_extern)
        continue;

      bool changed = true;
      while (changed) {
        changed = false;
        changed |= constant_folding(*fn);
      }
    }
  }
};

#endif // SRC_HIR_OPT_OPTIMIZATION_PIPELINE_HPP
