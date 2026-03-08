#ifndef SRC_CODEGEN_AARCH64_EMIT_HPP
#define SRC_CODEGEN_AARCH64_EMIT_HPP

#include "../../lir/lir.hpp"
#include "aarch64_defs.hpp"
#include <format>
#include <sstream>
#include <string>

namespace aarch64 {

class AsmEmitter {
  std::ostringstream out;
  const TargetInfo &target;

  // Emit a register name (w for 32-bit, x for 64-bit)
  // For now assume 32-bit (int) for everything
  std::string wreg(lir::Register reg) {
    assert(reg.is_physical());
    if (reg.id <= 28)
      return std::format("x{}", reg.id);
    if (reg.id == 29)
      return "x29";
    if (reg.id == 30)
      return "x30";
    if (reg.id == 32)
      return "xzr";
    return std::format("x{}", reg.id);
  }

  std::string xreg(lir::Register reg) {
    assert(reg.is_physical());
    if (reg.id <= 28)
      return std::format("x{}", reg.id);
    if (reg.id == 29)
      return "fp";
    if (reg.id == 30)
      return "lr";
    if (reg.id == 31)
      return "sp";
    if (reg.id == 32)
      return "xzr";
    return std::format("x{}", reg.id);
  }

  std::string operand(lir::Operand op) {
    if (op.is_reg())
      return wreg(op.get_reg());
    if (op.is_imm())
      return std::format("#{}", op.get_imm());
    if (op.is_block())
      return std::format(".{}", op.get_block()->label);
    return "???";
  }

  std::string cond_code(lir::CmpPredicate pred) {
    switch (pred) {
    case lir::CmpPredicate::EQ:
      return "eq";
    case lir::CmpPredicate::NE:
      return "ne";
    case lir::CmpPredicate::SLT:
      return "lt";
    case lir::CmpPredicate::SLE:
      return "le";
    case lir::CmpPredicate::SGT:
      return "gt";
    case lir::CmpPredicate::SGE:
      return "ge";
    case lir::CmpPredicate::ULT:
      return "lo";
    case lir::CmpPredicate::ULE:
      return "ls";
    case lir::CmpPredicate::UGT:
      return "hi";
    case lir::CmpPredicate::UGE:
      return "hs";
    }
    return "??";
  }

  void emit_line(const std::string &line) { out << "    " << line << "\n"; }

  void emit_label(const std::string &label) { out << "." << label << ":\n"; }

  void emit_instruction(lir::Instruction *inst) {
    switch (inst->opcode) {
    case lir::Opcode::Mov: {
      auto dst = inst->def(0);
      auto src = inst->use(0);
      if (src.is_imm()) {
        emit_line(
            std::format("mov {}, #{}", wreg(dst.get_reg()), src.get_imm()));
      } else {
        emit_line(std::format("mov {}, {}", wreg(dst.get_reg()), operand(src)));
      }
      break;
    }
    case lir::Opcode::Copy: {
      auto dst = inst->def(0);
      auto src = inst->use(0);
      if (src.is_imm()) {
        emit_line(
            std::format("mov {}, #{}", wreg(dst.get_reg()), src.get_imm()));
      } else if (dst.get_reg() != src.get_reg()) {
        emit_line(std::format("mov {}, {}", wreg(dst.get_reg()), operand(src)));
      }
      // Skip if src == dst (coalesced)
      break;
    }
    case lir::Opcode::Add: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("add {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Sub: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("sub {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Mul: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      // MUL cannot take immediates
      emit_line(std::format("mul {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::SDiv: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      // SDIV cannot take immediates
      emit_line(std::format("sdiv {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Neg: {
      auto dst = wreg(inst->def(0).get_reg());
      auto src = operand(inst->use(0));
      emit_line(std::format("neg {}, {}", dst, src));
      break;
    }
    case lir::Opcode::And: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("and {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Or: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("orr {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Xor: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("eor {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Shl: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("lsl {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::AShr: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("asr {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::LShr: {
      auto dst = wreg(inst->def(0).get_reg());
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("lsr {}, {}, {}", dst, lhs, rhs));
      break;
    }
    case lir::Opcode::Cmp: {
      auto lhs = operand(inst->use(0));
      auto rhs = operand(inst->use(1));
      emit_line(std::format("cmp {}, {}", lhs, rhs));
      break;
    }
    case lir::Opcode::CSet: {
      auto dst = wreg(inst->def(0).get_reg());
      auto cc = cond_code(inst->predicate.value());
      emit_line(std::format("cset {}, {}", dst, cc));
      break;
    }
    case lir::Opcode::Load: {
      auto dst = wreg(inst->def(0).get_reg());
      auto addr = xreg(inst->use(0).get_reg());
      emit_line(std::format("ldr {}, [{}]", dst, addr));
      break;
    }
    case lir::Opcode::Store: {
      auto addr = xreg(inst->use(0).get_reg());
      auto src = wreg(inst->use(1).get_reg());
      emit_line(std::format("str {}, [{}]", src, addr));
      break;
    }
    case lir::Opcode::Jump: {
      auto target_block = inst->use(0).get_block();
      emit_line(std::format("b .{}", target_block->label));
      break;
    }
    case lir::Opcode::CondJump: {
      auto cc = cond_code(inst->predicate.value());
      auto true_block = inst->operands[0].get_block();
      auto false_block = inst->operands[1].get_block();
      emit_line(std::format("b.{} .{}", cc, true_block->label));
      emit_line(std::format("b .{}", false_block->label));
      break;
    }
    case lir::Opcode::Call: {
      emit_line(std::format("bl _{}", inst->callee));
      break;
    }
    case lir::Opcode::Ret: {
      emit_line("ret");
      break;
    }
    }
  }

  // Determine which callee-saved registers are used
  std::vector<lir::Register> used_callee_saved(lir::Function *func) const {
    std::vector<lir::Register> used;
    for (size_t i = 0; i < target.num_callee_saved; i++) {
      auto reg = target.callee_saved[i];
      for (auto *mbb : func->blocks) {
        for (auto *inst : mbb->instructions) {
          for (auto &op : inst->operands) {
            if (op.is_reg() && op.get_reg() == reg) {
              used.push_back(reg);
              goto next_reg;
            }
          }
        }
      }
    next_reg:;
    }
    return used;
  }

  void emit_prologue(lir::Function *func) {
    auto callee_regs = used_callee_saved(func);

    // Always save fp and lr if function has calls
    bool save_fp_lr = func->has_calls;

    // Calculate frame size: callee-saved pairs + alignment
    size_t frame_size = 0;
    if (save_fp_lr)
      frame_size += 16;
    // Round callee-saved to pairs
    size_t callee_pairs = (callee_regs.size() + 1) / 2;
    frame_size += callee_pairs * 16;

    // Align to 16 bytes
    frame_size = (frame_size + 15) & ~15ULL;

    if (frame_size == 0)
      return;

    // Save fp and lr
    if (save_fp_lr) {
      emit_line(std::format("stp fp, lr, [sp, #-{}]!", frame_size));
      emit_line("mov fp, sp");
    } else if (frame_size > 0) {
      emit_line(std::format("sub sp, sp, #{}", frame_size));
    }

    // Save callee-saved registers
    size_t offset = save_fp_lr ? 16 : 0;
    for (size_t i = 0; i < callee_regs.size(); i += 2) {
      if (i + 1 < callee_regs.size()) {
        emit_line(std::format("stp {}, {}, [sp, #{}]", xreg(callee_regs[i]),
                              xreg(callee_regs[i + 1]), offset));
      } else {
        emit_line(
            std::format("str {}, [sp, #{}]", xreg(callee_regs[i]), offset));
      }
      offset += 16;
    }
  }

  void emit_epilogue(lir::Function *func) {
    auto callee_regs = used_callee_saved(func);

    bool save_fp_lr = func->has_calls;

    size_t frame_size = 0;
    if (save_fp_lr)
      frame_size += 16;
    size_t callee_pairs = (callee_regs.size() + 1) / 2;
    frame_size += callee_pairs * 16;
    frame_size = (frame_size + 15) & ~15ULL;

    if (frame_size == 0)
      return;

    // Restore callee-saved registers
    size_t offset = save_fp_lr ? 16 : 0;
    for (size_t i = 0; i < callee_regs.size(); i += 2) {
      if (i + 1 < callee_regs.size()) {
        emit_line(std::format("ldp {}, {}, [sp, #{}]", xreg(callee_regs[i]),
                              xreg(callee_regs[i + 1]), offset));
      } else {
        emit_line(
            std::format("ldr {}, [sp, #{}]", xreg(callee_regs[i]), offset));
      }
      offset += 16;
    }

    // Restore fp and lr
    if (save_fp_lr) {
      emit_line(std::format("ldp fp, lr, [sp], #{}", frame_size));
    } else {
      emit_line(std::format("add sp, sp, #{}", frame_size));
    }
  }

public:
  explicit AsmEmitter(const TargetInfo &target) : target(target) {}

  std::string emit_module(lir::Module *module) {
    // Emit text section
    out << ".section __TEXT,__text\n";
    out << ".align 4\n\n";

    for (auto *func : module->functions) {
      emit_function(func);
      out << "\n";
    }

    return out.str();
  }

  void emit_function(lir::Function *func) {
    // Export symbol
    if (func->is_extern) {
      out << ".extern _" << func->name << "\n";
      return;
    }
    out << ".globl _" << func->name << "\n";
    out << "_" << func->name << ":\n";

    // Prologue
    emit_prologue(func);

    // Emit blocks
    bool first_block = true;
    for (auto *mbb : func->blocks) {
      // Skip label for entry block (already emitted as function name)
      if (!first_block) {
        emit_label(mbb->label);
      }
      first_block = false;

      for (auto *inst : mbb->instructions) {
        // Replace ret with epilogue + ret
        if (inst->opcode == lir::Opcode::Ret) {
          emit_epilogue(func);
          emit_instruction(inst);
        } else {
          emit_instruction(inst);
        }
      }
    }
  }
};

} // namespace aarch64

#endif
