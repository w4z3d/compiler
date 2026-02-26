#ifndef SRC_HIR_HIR_BUILDER_HPP
#define SRC_HIR_HIR_BUILDER_HPP
#include "../alloc/arena.hpp"
#include "hir.hpp"
#include "hir_type.hpp"
#include <cstddef>
#include <string_view>

struct HIRBuilder {

  explicit HIRBuilder(hir::Module &m)
      : module(m), context(m.types), arena(arena::Arena{}) {}

  void set_insert_point(hir::BasicBlock *block);
  void set_insert_before(hir::Instruction *instruction);

  // Arithmetic
  [[maybe_unused]] hir::Value *build_add(hir::Value *lhs, hir::Value *rhs,
                                         std::string_view name = "");
  [[maybe_unused]] hir::Value *build_sub(hir::Value *lhs, hir::Value *rhs,
                                         std::string_view name = "");
  [[maybe_unused]] hir::Value *build_mul(hir::Value *lhs, hir::Value *rhs,
                                         std::string_view name = "");
  [[maybe_unused]] hir::Value *build_sdiv(hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_udiv(hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_srem(hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_urem(hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_neg(hir::Value *lhs,
                                         std::string_view name = "");

  // Bitwise
  [[maybe_unused]] hir::Value *build_and(hir::Value *lhs, hir::Value *rhs,
                                         std::string_view name = "");
  [[maybe_unused]] hir::Value *build_or(hir::Value *lhs, hir::Value *rhs,
                                        std::string_view name = "");
  [[maybe_unused]] hir::Value *build_xor(hir::Value *lhs, hir::Value *rhs,
                                         std::string_view name = "");
  [[maybe_unused]] hir::Value *build_shl(hir::Value *lhs, hir::Value *rhs,
                                         std::string_view name = "");
  [[maybe_unused]] hir::Value *build_lshr(hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_ashr(hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_not(hir::Value *lhs,
                                         std::string_view name = "");

  // Comparisons always return i1 (a 1 bit integer type)
  [[maybe_unused]] hir::Value *build_icmp(hir::ICmpPredicate predicate,
                                          hir::Value *lhs, hir::Value *rhs,
                                          std::string_view name = "");

  // Memory
  [[maybe_unused]] hir::Value *build_alloca(hir::type::Type *allocated_type,
                                            std::string_view name = "");
  [[maybe_unused]] hir::Value *build_load(hir::type::Type *loaded_type,
                                          hir::Value *ptr,
                                          std::string_view name = "");
  void build_store(hir::Value *value, hir::Value *ptr);
  [[maybe_unused]] hir::Value *
  build_gep(hir::type::Type *base_type, hir::Value *ptr,
            const std::vector<hir::Value *> &indices,
            std::string_view name = "");
  [[maybe_unused]] hir::Value *build_trunc(hir::Value *v,
                                           hir::type::IntegerType *to,
                                           std::string_view name = "");

  [[maybe_unused]] hir::Value *build_zext(hir::Value *v,
                                          hir::type::IntegerType *to,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_sext(hir::Value *v,
                                          hir::type::IntegerType to,
                                          std::string_view name = "");
  [[maybe_unused]] hir::Value *build_ptr_to_int(hir::Value *v,
                                                hir::type::IntegerType *to,
                                                std::string_view name = "");
  [[maybe_unused]] hir::Value *build_int_to_ptr(hir::Value *v,
                                                hir::type::IntegerType *to,
                                                std::string_view name = "");

  // Control Flow
  void build_br(hir::BasicBlock *target);
  void build_cond_br(hir::Value *cond, hir::BasicBlock *true_block,
                     hir::BasicBlock *false_block);
  void build_ret(hir::Value *v = nullptr);

  [[maybe_unused]] hir::Value *build_call(hir::Function *callee,
                                          std::vector<hir::Value *> args,
                                          std::string_view name = "");
  [[maybe_unused]] hir::PhiNode *build_phi(hir::type::Type *type,
                                           std::string_view name = "");

  arena::Arena arena;
  hir::Module &module;
  hir::type::TypeContext &context;

private:
  hir::BasicBlock *insert_block_ = nullptr;
  std::list<hir::Instruction *>::iterator insert_point_;
  [[maybe_unused]] hir::Instruction *insert(hir::Instruction *i);
};

#endif // SRC_HIR_HIR_BUILDER_HPP
#ifndef SRC_HIR_HIR_BUILDER_HPP
#define SRC_HIR_HIR_BUILDER_HPP

#endif // SRC_HIR_HIR_BUILDER_HPP
#ifndef SRC_HIR_HIR_BUILDER_HPP
#define SRC_HIR_HIR_BUILDER_HPP

#endif // SRC_HIR_HIR_BUILDER_HPP
#ifndef __SRC_HIR_HIR_BUILDER_HPP
#define __SRC_HIR_HIR_BUILDER_HPP

#endif // __SRC_HIR_HIR_BUILDER_HPP
