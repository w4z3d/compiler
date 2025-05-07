#include "ast.hpp"

void Declaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void FunctionDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ParameterDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Statement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CompoundStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ReturnStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void AssertStmt::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void VariableDeclarationStatement::accept(ASTVisitor &visitor) {
  visitor.visit(*this);
}
void ExpressionStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void UnaryMutationStatement::accept(ASTVisitor &visitor) {
  visitor.visit(*this);
}
void AssignmentStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void IfStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ForStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void WhileStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void ErrorStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Expression::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void NumericExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CallExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void StringLiteralExpr::accept(ASTVisitor &visitor) { visitor.visit(*this); }
void CharLiteralExpr::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void BoolConstExpr::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void NullExpr::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void ParenthesisExpression::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void BinaryOperatorExpression::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void UnaryOperatorExpression::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void VarExpr::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void ArrayAccessExpr::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void PointerAccessExpr::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void FieldAccessExpr::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void AllocArrayExpression::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void AllocExpression::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void TernaryExpression::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}


void TranslationUnit::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BuiltinType::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void NamedType::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void StructType::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void PointerType::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }
void ArrayType::accept(struct ASTVisitor &visitor) { visitor.visit(*this); }

void VariableLValue::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void DereferenceLValue::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void ArrayAccessLValue::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void FieldAccessLValue::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}
void PointerAccessLValue::accept(struct ASTVisitor &visitor) {
  visitor.visit(*this);
}

int precedenceFromBinOp(BinaryOperator binOp) {
  switch (binOp) {
  case BinaryOperator::Add:
  case BinaryOperator::Sub:
    return 10;
  case BinaryOperator::Mult:
  case BinaryOperator::Div:
  case BinaryOperator::Modulo:
    return 11;
  case BinaryOperator::Equal:
  case BinaryOperator::NotEqual:
    return 7;
  case BinaryOperator::LessThan:
  case BinaryOperator::LessThanOrEqual:
  case BinaryOperator::GreaterThan:
  case BinaryOperator::GreaterThanOrEqual:
    return 8;
  case BinaryOperator::LogicalAnd:
    return 3;
  case BinaryOperator::LogicalOr:
    return 2;
  case BinaryOperator::BitwiseAnd:
    return 6;
  case BinaryOperator::BitwiseOr:
    return 4;
  case BinaryOperator::BitwiseXor:
    return 5;
  case BinaryOperator::ShiftLeft:
  case BinaryOperator::ShiftRight:
    return 9;
  default:
    return -1;
  }
}

std::string binOp2String(BinaryOperator binOp) {
  switch (binOp) {
  case BinaryOperator::Add:
    return "Add";
  case BinaryOperator::Sub:
    return "Sub";
  case BinaryOperator::Mult:
    return "Mult";
  case BinaryOperator::Div:
    return "Div";
  case BinaryOperator::Modulo:
    return "Modulo";
  case BinaryOperator::Equal:
    return "Equal";
  case BinaryOperator::NotEqual:
    return "NotEqual";
  case BinaryOperator::LessThan:
    return "LessThan";
  case BinaryOperator::LessThanOrEqual:
    return "LessThanOrEqual";
  case BinaryOperator::GreaterThan:
    return "GreaterThan";
  case BinaryOperator::GreaterThanOrEqual:
    return "GreaterThanOrEqual";
  case BinaryOperator::LogicalAnd:
    return "LogicalAnd";
  case BinaryOperator::LogicalOr:
    return "LogicalOr";
  case BinaryOperator::BitwiseAnd:
    return "BitwiseAnd";
  case BinaryOperator::BitwiseOr:
    return "BitwiseOr";
  case BinaryOperator::BitwiseXor:
    return "BitwiseXor";
  case BinaryOperator::ShiftLeft:
    return "ShiftLeft";
  case BinaryOperator::ShiftRight:
    return "ShiftRight";
  case BinaryOperator::FieldAccess:
    return "FieldAccess";
  case BinaryOperator::PointerAccess:
    return "PointerAccess";
  default:
    return "UnknownBinOperator";
  }
}

int precedenceFromUnOp(UnaryOperator unOp) {
  switch (unOp) {
  case UnaryOperator::LogicalNot:
  case UnaryOperator::BitwiseNot:
  case UnaryOperator::Neg:
  case UnaryOperator::Deref:
    return 12;
  default:
    return -1;
  }
}

std::string unOp2String(UnaryOperator unOp) {
  switch (unOp) {
  case UnaryOperator::LogicalNot:
    return "LogicalNot";
  case UnaryOperator::BitwiseNot:
    return "BitwiseNot";
  case UnaryOperator::Neg:
    return "Neg";
  case UnaryOperator::Deref:
    return "Deref";
  default:
    return "UnknownUnaryOperator";
  }
}

int precedenceFromAssmtOp(AssignmentOperator assmtOp) {
  switch (assmtOp) {
  case AssignmentOperator::Plus:
  case AssignmentOperator::Minus:
  case AssignmentOperator::Mult:
  case AssignmentOperator::Div:
  case AssignmentOperator::Modulo:
  case AssignmentOperator::LShift:
  case AssignmentOperator::RShift:
  case AssignmentOperator::BitwiseXor:
  case AssignmentOperator::BitwiseOr:
  case AssignmentOperator::BitwiseAnd:
  case AssignmentOperator::Equals:
    return 0;
  default:
    return -1;
  }
}

std::string assmtOp2String(AssignmentOperator assmtOp) {
  switch (assmtOp) {
  case AssignmentOperator::Plus:
    return "Plus";
  case AssignmentOperator::Minus:
    return "Minus";
  case AssignmentOperator::Mult:
    return "Mult";
  case AssignmentOperator::Div:
    return "Div";
  case AssignmentOperator::Modulo:
    return "Modulo";
  case AssignmentOperator::LShift:
    return "LShift";
  case AssignmentOperator::RShift:
    return "RShift";
  case AssignmentOperator::BitwiseXor:
    return "BitwiseXor";
  case AssignmentOperator::BitwiseOr:
    return "BitwiseOr";
  case AssignmentOperator::BitwiseAnd:
    return "BitwiseAnd";
  case AssignmentOperator::Equals:
    return "Equals";
  default:
    return "Unknown Assignment Operator";
  }
}

std::string builtin2String(Builtin type) {
  switch (type) {
  case Builtin::Int:
    return "int";
  case Builtin::Bool:
    return "bool";
  case Builtin::Char:
    return "char";
  case Builtin::String:
    return "string";
  case Builtin::Void:
    return "void";
  default:
    return "Unknown type";
  }
}

BinaryOperator binOpFromToken(token::TokenKind token) {
  switch (token) {
  case token::TokenKind::Plus:
    return BinaryOperator::Add;
  case token::TokenKind::Minus:
    return BinaryOperator::Sub;
  case token::TokenKind::Asterisk:
    return BinaryOperator::Mult;
  case token::TokenKind::Slash:
    return BinaryOperator::Div;
  case token::TokenKind::Percent:
    return BinaryOperator::Modulo;
  case token::TokenKind::Equals:
    return BinaryOperator::Equal;
  case token::TokenKind::BangEqual:
    return BinaryOperator::NotEqual;
  case token::TokenKind::LAngleBracket:
    return BinaryOperator::LessThan;
  case token::TokenKind::LessEqual:
    return BinaryOperator::LessThanOrEqual;
  case token::TokenKind::RAngleBracket:
    return BinaryOperator::GreaterThan;
  case token::TokenKind::GreaterEqual:
    return BinaryOperator::GreaterThanOrEqual;
  case token::TokenKind::AndAnd:
    return BinaryOperator::LogicalAnd;
  case token::TokenKind::PipePipe:
    return BinaryOperator::LogicalOr;
  case token::TokenKind::And:
    return BinaryOperator::BitwiseAnd;
  case token::TokenKind::Pipe:
    return BinaryOperator::BitwiseOr;
  case token::TokenKind::Caret:
    return BinaryOperator::BitwiseXor;
  case token::TokenKind::LAngleAngle:
    return BinaryOperator::ShiftLeft;
  case token::TokenKind::RAngleAngle:
    return BinaryOperator::ShiftRight;
  case token::TokenKind::Dot:
    return BinaryOperator::FieldAccess;
  case token::TokenKind::Arrow:
    return BinaryOperator::PointerAccess;
  default:
    return BinaryOperator::Unknown;
  }
}

UnaryOperator unOpFromToken(token::TokenKind token) {
  switch (token) {
  case token::TokenKind::Bang:
    return UnaryOperator::LogicalNot;
  case token::TokenKind::Tilde:
    return UnaryOperator::BitwiseNot;
  case token::TokenKind::Minus:
    return UnaryOperator::Neg;
  case token::TokenKind::Asterisk:
    return UnaryOperator::Deref;
  default:
    return UnaryOperator::Unknown;
  }
}

AssignmentOperator assmtOpFromToken(token::TokenKind token) {
  switch (token) {
  case token::TokenKind::PlusEquals:
    return AssignmentOperator::Plus;
  case token::TokenKind::MinusEquals:
    return AssignmentOperator::Minus;
  case token::TokenKind::PercentEquals:
    return AssignmentOperator::Modulo;
  case token::TokenKind::Equals:
    return AssignmentOperator::Equals;
  case token::TokenKind::AndEquals:
    return AssignmentOperator::BitwiseAnd;
  case token::TokenKind::PipeEquals:
    return AssignmentOperator::BitwiseOr;
  case token::TokenKind::CaretEquals:
    return AssignmentOperator::BitwiseXor;
  case token::TokenKind::LAngleAngleEquals:
    return AssignmentOperator::LShift;
  case token::TokenKind::RAngleAngleEquals:
    return AssignmentOperator::RShift;
  case token::TokenKind::AsteriskEquals:
    return AssignmentOperator::Mult;
  case token::TokenKind::SlashEquals:
    return AssignmentOperator::Div;
  default:
    return AssignmentOperator::Unknown;
  }
}

Builtin builtinFromToken(token::TokenKind token) {
  switch (token) {
  case token::TokenKind::Int:
    return Builtin::Int;
  case token::TokenKind::Bool:
    return Builtin::Bool;
  case token::TokenKind::String:
    return Builtin::String;
  case token::TokenKind::Char:
    return Builtin::Char;
  case token::TokenKind::Void:
    return Builtin::Void;
  default:
    return Builtin::Unknown;
  }
}
