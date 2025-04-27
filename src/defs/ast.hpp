#ifndef DEFS_AST_H
#define DEFS_AST_H
#include <memory>

struct ASTNode {};

using NodePtr = std::unique_ptr<ASTNode>;

struct Program {};

#endif // !DEFS_AST_H
