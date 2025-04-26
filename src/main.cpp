#include "defs/ast.hpp"
#include <memory>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
int main(int argc, char *argv[]) {
  auto lhs = std::make_shared<ast::LiteralExpr>("12");
  auto rhs = std::make_shared<ast::LiteralExpr>("22");
  auto bin_exp = std::make_shared<ast::BinaryExpression>("+", lhs, rhs);

  auto lhs1 = std::make_shared<ast::LiteralExpr>("1337");
  ast::BinaryExpression next_expression{"*", lhs1, bin_exp};

  ast::PrintVisitor visitor{};
  next_expression.accept(visitor);
  spdlog::log(spdlog::level::info, "{}", visitor.content);
  return 0;
}
