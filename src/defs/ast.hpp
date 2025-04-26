#ifndef DEFS_AST_H
#define DEFS_AST_H

#include <memory>
#include <string>
#include <utility>

namespace ast {

class LiteralExpr;
class BinaryExpression;

class ASTVisitor {
public:
  virtual void visit(LiteralExpr &expression) = 0;
  virtual void visit(BinaryExpression &expression) = 0;
};

class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual void accept(class ASTVisitor &visitor) = 0;
};

using ASTNodePtr = std::shared_ptr<ASTNode>;

class LiteralExpr : public ASTNode {
public:
  std::string value;

  explicit LiteralExpr(std::string val) : value(std::move(val)) {}

  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

class BinaryExpression : public ASTNode {
public:
  std::string op;
  ASTNodePtr lhs;
  ASTNodePtr rhs;

  BinaryExpression(std::string op, ASTNodePtr lhs, ASTNodePtr rhs)
      : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs)) {}

  void accept(ASTVisitor &visitor) override { visitor.visit(*this); }
};

class PrintVisitor : public ASTVisitor {
public:
  std::string content;
  void visit(BinaryExpression &expression);

  void visit(LiteralExpr &expression);
};

} // namespace ast

#endif // !DEFS_AST_H
