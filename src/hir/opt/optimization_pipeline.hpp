#ifndef SRC_HIR_OPT_OPTIMIZATION_PIPELINE_HPP
#define SRC_HIR_OPT_OPTIMIZATION_PIPELINE_HPP
#include "../hir.hpp"
bool constant_folding(hir::Function *func) {
  bool changed = false;

  for (auto *bb : func->blocks) {
    for (auto *inst : bb->instructions) {
      // Skip non-binary ops
      if (inst->operand_count() != 2)
        continue;

      auto *lhs = dynamic_cast<hir::ConstantInt *>(inst->operand(0));
      auto *rhs = dynamic_cast<hir::ConstantInt *>(inst->operand(1));
      if (!lhs || !rhs)
        continue;

      int64_t result;
      switch (inst->opcode) {
      case hir::Opcode::Add:
        result = lhs->signed_value() + rhs->signed_value();
        break;
      case hir::Opcode::Sub:
        result = lhs->signed_value() - rhs->signed_value();
        break;
      case hir::Opcode::Mul:
        result = lhs->signed_value() * rhs->signed_value();
        break;
      case hir::Opcode::SDiv:
        if (rhs->signed_value() == 0)
          continue;
        result = lhs->signed_value() / rhs->signed_value();
        break;
      case hir::Opcode::SRem:
        if (rhs->signed_value() == 0)
          continue;
        result = lhs->signed_value() % rhs->signed_value();
        break;
      case hir::Opcode::And:
        result = lhs->signed_value() & rhs->signed_value();
        break;
      case hir::Opcode::Or:
        result = lhs->signed_value() | rhs->signed_value();
        break;
      case hir::Opcode::Xor:
        result = lhs->signed_value() ^ rhs->signed_value();
        break;
      case hir::Opcode::Shl:
        result = lhs->signed_value() << rhs->signed_value();
        break;
      case hir::Opcode::AShr:
        result = lhs->signed_value() >> rhs->signed_value();
        break;
      case hir::Opcode::LShr:
        result =
            static_cast<int64_t>(lhs->signed_value()) >> rhs->signed_value();
        break;
      case hir::Opcode::ICmp: {
        bool cmp_result;
        switch (inst->predicate.value()) {
        case hir::ICmpPredicate::EQ:
          cmp_result = lhs->signed_value() == rhs->signed_value();
          break;
        case hir::ICmpPredicate::NE:
          cmp_result = lhs->signed_value() != rhs->signed_value();
          break;
        case hir::ICmpPredicate::SLT:
          cmp_result = lhs->signed_value() < rhs->signed_value();
          break;
        case hir::ICmpPredicate::SLE:
          cmp_result = lhs->signed_value() <= rhs->signed_value();
          break;
        case hir::ICmpPredicate::SGT:
          cmp_result = lhs->signed_value() > rhs->signed_value();
          break;
        case hir::ICmpPredicate::SGE:
          cmp_result = lhs->signed_value() >= rhs->signed_value();
          break;
        default:
          continue;
        }
        result = cmp_result ? 1 : 0;
        break;
      }
      default:
        continue;
      }

      auto *constant = func->parent->const_int(
          dynamic_cast<hir::type::IntegerType *>(inst->type), result);
      inst->replace_all_uses_with(constant);
      changed = true;
    }
  }

  return changed;
}
bool dead_code_elimination(hir::Function *func) {
  bool changed = false;

  for (auto *bb : func->blocks) {
    auto &instrs = bb->instructions;
    instrs.erase(std::remove_if(instrs.begin(), instrs.end(),
                                [&](hir::Instruction *inst) {
                                  if (inst->is_terminator())
                                    return false;
                                  if (inst->has_side_effects())
                                    return false;
                                  if (inst->users.empty()) {
                                    changed = true;
                                    return true;
                                  }
                                  return false;
                                }),
                 instrs.end());
  }

  return changed;
}
struct OptPipeline {
  void optimize(hir::Function *func) {
    bool changed = true;
    while (changed) {
      changed = false;
      changed |= constant_folding(func);
      changed |= dead_code_elimination(func);
    }
  }
};

#endif // SRC_HIR_OPT_OPTIMIZATION_PIPELINE_HPP
