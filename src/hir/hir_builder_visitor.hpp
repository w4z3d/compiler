#ifndef SRC_HIR_HIR_BUILDER_VISITOR_HPP
#define SRC_HIR_HIR_BUILDER_VISITOR_HPP

#include "../analysis/symbol.hpp"
#include "../defs/ast.hpp"
#include "../report/report_builder.hpp"
#include "hir_builder.hpp"
#include "hir_type.hpp"
#include <stack>
#include <unordered_map>
#include <unordered_set>

struct HIRBuilderVisitor : ASTVisitor {
  hir::Module &module;
  HIRBuilder builder;
  hir::type::TypeContext &types;
  std::shared_ptr<SymbolTable> symbol_table;
  std::shared_ptr<DiagnosticEmitter> diagnostics; // reference — owned by caller

  // Traversal state
  hir::Function *current_function = nullptr;
  hir::BasicBlock *current_block = nullptr;
  std::stack<hir::Value *> value_stack;
  // Maps symbol id → alloca instruction for that variable
  // Used by loads and stores to find where a variable lives in memory
  std::unordered_map<size_t, hir::Value *> var_addresses;
  // SSA construction state
  std::unordered_map<size_t,
                     std::unordered_map<hir::BasicBlock *, hir::Value *>>
      current_defs;
  std::unordered_set<hir::BasicBlock *> sealed_blocks;
  std::unordered_map<hir::BasicBlock *,
                     std::unordered_map<size_t, hir::PhiNode *>>
      incomplete_phis;
  std::unordered_set<hir::PhiNode *> phis_being_removed;

public:
  explicit HIRBuilderVisitor(hir::Module &mod,
                             std::shared_ptr<SymbolTable> sym_table,
                             std::shared_ptr<DiagnosticEmitter> diag)
      : module(mod), builder(mod), types(mod.types),
        symbol_table(std::move(sym_table)), diagnostics(std::move(diag)) {}

  hir::Module &get_module() { return module; }

private:
  // SSA helpers
  void write_variable(size_t symbol_id, hir::BasicBlock *bb, hir::Value *val);
  hir::Value *read_variable(size_t symbol_id, hir::BasicBlock *bb);
  hir::Value *read_variable_recursive(size_t symbol_id, hir::BasicBlock *bb);
  hir::Value *try_remove_trivial_phi(hir::PhiNode *phi);
  hir::Value *
  add_phi_operands(size_t variable, hir::PhiNode *phi,
                   const std::vector<hir::BasicBlock *> &predecessors);
  void seal_block(hir::BasicBlock *bb);

  // Type helpers
  hir::type::Type *type_of_symbol(size_t symbol_id);
  hir::type::Type *lower_type(const type::Type *ast_type);
  size_t size_of(hir::type::Type *type);

  void set_insert_point(hir::BasicBlock *bb) {
    current_block = bb;
    builder.set_insert_point(bb);
  }

  inline hir::Value *pop_stack() {
    if (value_stack.empty()) {
      std::cerr << "ERROR: value stack underflow — "
                   "a visit method forgot to push a value\n";
      // Print backtrace hint
      assert(false && "value stack underflow");
    }
    hir::Value *val = value_stack.top();
    value_stack.pop();

    if (!val) {
      assert(false && "null value popped from stack");
    }

    return val;
  }

  void reset_ssa_state() {
    // Stack should be empty at end of function — if not, something
    // pushed a value without popping it
    if (!value_stack.empty()) {
      std::cerr << "WARNING: value stack not empty at end of function, size="
                << value_stack.size() << std::endl;
      // Print what's on the stack
      auto copy = value_stack;
      while (!copy.empty()) {
        std::cerr << "  " << copy.top()->to_string() << std::endl;
        copy.pop();
      }
    }

    current_defs.clear();
    sealed_blocks.clear();
    incomplete_phis.clear();
    current_function = nullptr;
    current_block = nullptr;
    // Don't pop — just reconstruct the stack cleanly
    value_stack = std::stack<hir::Value *>{};
  }

  // Declarations
  void visit(Typedef &typedef_) override;
  void visit(TranslationUnit &unit) override;
  void visit(FunctionDeclaration &decl) override;
  void visit(ParameterDeclaration &decl) override;
  void visit(StructDeclaration &decl) override;

  // Statements
  void visit(CompoundStmt &stmt) override;
  void visit(ReturnStmt &stmt) override;
  void visit(ErrorStatement &stmt) override;
  void visit(AssertStmt &stmt) override;
  void visit(IfStatement &stmt) override;
  void visit(ForStatement &stmt) override;
  void visit(WhileStatement &stmt) override;
  void visit(VariableDeclarationStatement &stmt) override;
  void visit(AssignmentStatement &stmt) override;
  void visit(UnaryMutationStatement &stmt) override;
  void visit(ExpressionStatement &stmt) override;

  // Expressions
  void visit(CallExpr &expr) override;
  void visit(NumericExpr &expr) override;
  void visit(StringLiteralExpr &expr) override;
  void visit(CharLiteralExpr &expr) override;
  void visit(BoolConstExpr &expr) override;
  void visit(NullExpr &expr) override;
  void visit(ParenthesisExpression &expr) override;
  void visit(VarExpr &expr) override;
  void visit(UnaryOperatorExpression &expr) override;
  void visit(BinaryOperatorExpression &expr) override;
  void visit(TernaryExpression &expr) override;
  void visit(ArrayAccessExpr &expr) override;
  void visit(FieldAccessExpr &expr) override;
  void visit(PointerAccessExpr &expr) override;
  void visit(AllocExpression &expr) override;
  void visit(AllocArrayExpression &expr) override;

  // Type annotations
  void visit(BuiltinTypeAnnotation &type) override;
  void visit(NamedTypeAnnotation &type) override;
  void visit(StructTypeAnnotation &type) override;
  void visit(PointerTypeAnnotation &type) override;
  void visit(ArrayTypeAnnotation &type) override;

  // LValues
  void visit(VariableLValue &val) override;
  void visit(DereferenceLValue &val) override;
  void visit(FieldAccessLValue &val) override;
  void visit(PointerAccessLValue &val) override;
  void visit(ArrayAccessLValue &val) override;
};

#endif // SRC_HIR_HIR_BUILDER_VISITOR_HPP
