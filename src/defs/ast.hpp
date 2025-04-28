#ifndef DEFS_AST_H
#define DEFS_AST_H

#include <string>
struct SourceLocation {
  std::string file_name;
  int line;
  int column;

  SourceLocation(std::string file_name = "", int line = 0, int column = 0)
      : file_name(file_name), line(line), column(column) {}
};

class ASTNode {
protected:
  SourceLocation location;

public:
  explicit ASTNode(SourceLocation loc = {}) : location(std::move(loc)) {}
  virtual ~ASTNode() = default;

  const SourceLocation &getLocation() const { return location; }

  virtual void accept(class ASTVisitor &visitor) = 0;
};

class TranslationUnit {};

#endif // !DEFS_AST_H
