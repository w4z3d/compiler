#include "parser.hpp"
#include "spdlog/common.h"

// Grammar reference:
// https://c0.cs.cmu.edu/docs/c0-reference.pdf#subsection.14.2

LValue *Parser::parse_lvalue() {
  if (is_next(token::TokenKind::Asterisk)) {
    const auto star = expect(token::TokenKind::Asterisk);
    const auto operand = parse_lvalue();
    const auto lv = arena.create<DereferenceLValue>(operand,
                                                    SourceLocation{
                                                        lexer.get_file_name(),
                                                        std::get<0>(star->span.start),
                                                        std::get<1>(star->span.start)
                                                    });
    return parse_lvalue_tail(lv);
  }
  else if (is_next(token::TokenKind::LParen)) {
    const auto lp = expect(token::TokenKind::LParen);
    const auto lv = parse_lvalue();
    const auto rp = expect(token::TokenKind::RParen);
    return parse_lvalue_tail(lv);
  }
  else {
    const auto iden = expect(token::TokenKind::Identifier);
    const auto lv = arena.create<VariableLValue>(iden->text,
                                                 SourceLocation{
                                                     lexer.get_file_name(),
                                                     std::get<0>(iden->span.start),
                                                     std::get<1>(iden->span.start)
                                                 });
    return parse_lvalue_tail(lv);
  }
}

LValue *Parser::parse_lvalue_tail(LValue *lvalue) {
  if (is_next(token::TokenKind::Dot)) {
    const auto dot = expect(token::TokenKind::Dot);
    const auto fid = expect(token::TokenKind::Identifier);
    const auto lv = arena.create<FieldAccessLValue>(lvalue, fid->text,
                                                    SourceLocation{
                                                        lexer.get_file_name(),
                                                        std::get<0>(dot->span.start),
                                                        std::get<1>(dot->span.start)
                                                    });
    return parse_lvalue_tail(lv);
  }
  else if (is_next(token::TokenKind::Arrow)) {
    const auto dot = expect(token::TokenKind::Arrow);
    const auto fid = expect(token::TokenKind::Identifier);
    const auto lv = arena.create<PointerAccessLValue>(lvalue, fid->text,
                                                    SourceLocation{
                                                        lexer.get_file_name(),
                                                        std::get<0>(dot->span.start),
                                                        std::get<1>(dot->span.start)
                                                    });
    return parse_lvalue_tail(lv);
  }
  else if (is_next(token::TokenKind::LBrace)) {
    const auto lb = expect(token::TokenKind::LBrace);
    const auto expr = parse_expression();
    const auto rb = expect(token::TokenKind::RBrace);
    const auto lv = arena.create<ArrayAccessLValue>(lvalue, expr,
                                                    SourceLocation{
                                                        lexer.get_file_name(),
                                                        std::get<0>(lb->span.start),
                                                        std::get<1>(lb->span.start)
                                                    });
    return parse_lvalue_tail(lv);
  }
  else {
    return lvalue;
  }
}

Type *Parser::parse_type() {
  if (is_next(token::TokenKind::Identifier)) {
    return parse_type_tail(parse_named_type());
  }
  else if (is_next(token::TokenKind::Struct)) {
    return parse_type_tail(parse_struct_type());
  }
  const auto next_token = peek();
  const auto builtin = builtinFromToken(next_token.kind);
  if (builtin != Builtin::Unknown) {
    return parse_type_tail(parse_builtin_type(next_token.kind));
  }
  else {
    throw ParseError(std::format("Expected Type, but next token was {}", next_token.text));
  }
}

BuiltinType *Parser::parse_builtin_type(token::TokenKind type) {
  const auto builtin = expect(type);
  const auto tp = arena.create<BuiltinType>(builtinFromToken(builtin->kind),
                                           SourceLocation{
                                               lexer.get_file_name(),
                                               std::get<0>(builtin->span.start),
                                               std::get<1>(builtin->span.start)
                                           });
  return tp;
}

StructType *Parser::parse_struct_type() {
  const auto str = expect(token::TokenKind::Struct);
  const auto iden = expect(token::TokenKind::Identifier);
  const auto tp = arena.create<StructType>(iden->text,
                                          SourceLocation{
                                              lexer.get_file_name(),
                                              std::get<0>(str->span.start),
                                              std::get<1>(str->span.start)
                                          });
  return tp;
}

NamedType *Parser::parse_named_type() {
  const auto iden = expect(token::TokenKind::Identifier);
  const auto tp = arena.create<NamedType>(iden->text,
                                          SourceLocation{
                                              lexer.get_file_name(),
                                              std::get<0>(iden->span.start),
                                              std::get<1>(iden->span.start)
                                          });
  return tp;
}

Type *Parser::parse_type_tail(Type *type) {
  if (is_next(token::TokenKind::Asterisk)) {
    const auto star = expect(token::TokenKind::Asterisk);
    const auto tp = arena.create<PointerType>(type, SourceLocation{
                                                      lexer.get_file_name(),
                                                      std::get<0>(star->span.start),
                                                      std::get<1>(star->span.start)
                                                  });
    return parse_type_tail(tp);
  }
  else if (is_next(token::TokenKind::LBracket)) {
    const auto first = expect(token::TokenKind::LBracket);
    const auto second = expect(token::TokenKind::RBracket);
    const auto tp = arena.create<ArrayType>(type, SourceLocation{
                                                      lexer.get_file_name(),
                                                      std::get<0>(first->span.start),
                                                      std::get<1>(first->span.start)
                                                  });
    return parse_type_tail(tp);
  }
  else {
    return type;
  }
}

Expression *Parser::parse_expression() {
  const auto next_token = peek();

  // Single token
  switch (next_token.kind) {
  case token::TokenKind::NumberLiteral:
    printf("test");
    return parse_expr_tail(parse_integer_literal());
  case token::TokenKind::StringLiteral:
    return parse_expr_tail(parse_string_literal());
  case token::TokenKind::CharLiteral:
    return parse_expr_tail(parse_char_literal());
  case token::TokenKind::True:
  case token::TokenKind::False:
    return parse_expr_tail(parse_bool_const());
  case token::TokenKind::Null:
    return parse_expr_tail(parse_null_expr());
  case token::TokenKind::LParen:
    return parse_expr_tail(parse_paren_expr());
  case token::TokenKind::Alloc:
    // TODO: needs type parsing
    break;
  case token::TokenKind::AllocArray:
    // TODO:
    break;
  default:
    break;
  }

  if (token::unary_ops.contains(peek().kind)) {
    return parse_expr_tail(parse_unary_op_expr());
  }

  if (check_sequence({token::TokenKind::Identifier, token::TokenKind::LParen})) {
    return parse_expr_tail(parse_call_expression());
  }
  else if (is_next(token::TokenKind::Identifier)) {
    return parse_expr_tail(parse_var_expr());
  }
  else {
    throw ParseError(std::format("Expected Expression, but next token was {}", peek().text));
  }
}

UnaryOperatorExpression *Parser::parse_unary_op_expr() {
  const auto opToken = next_token();
  const auto unOp = unOpFromToken(opToken.kind);
  if (unOp == UnaryOperator::Unknown) {
    throw std::runtime_error("lol du bist cooked (token::unary_ops enthält ein token, "
                             "das in func unOpFromToken(...) nicht als UnaryOperator aufgeführt ist)");
  }
  Expression *expr = parse_expression();
  const auto unaryExpr = arena.create<UnaryOperatorExpression>(expr, unOp,
                                                               SourceLocation{
                                                                   lexer.get_file_name(),
                                                                   std::get<0>(opToken.span.start),
                                                                   std::get<1>(opToken.span.start)
                                                               });
  return unaryExpr;
}

ParenthesisExpression *Parser::parse_paren_expr() {
  const auto lParen = expect(token::TokenKind::LParen);
  Expression *expr = parse_expression();
  const auto rParen = expect(token::TokenKind::RParen);
  const auto parenExpr = arena.create<ParenthesisExpression>(expr,
                                                             SourceLocation{
                                                                 lexer.get_file_name(), std::get<0>(lParen->span.start),
                                                                 std::get<1>(lParen->span.start)
                                                             });
  return parenExpr;
}

NullExpr *Parser::parse_null_expr() {
  const auto tok = expect(token::TokenKind::Null);
  return arena.create<NullExpr>(SourceLocation{
      lexer.get_file_name(), std::get<0>(tok->span.start),
      std::get<1>(tok->span.start)
  });
}

VarExpr *Parser::parse_var_expr() {
  const auto var = expect(token::TokenKind::Identifier);
  const auto expr = arena.create<VarExpr>(var->text,
                                          SourceLocation{
                                              lexer.get_file_name(), std::get<0>(var->span.start),
                                              std::get<1>(var->span.start)
                                          });
  return expr;
}

BoolConstExpr *Parser::parse_bool_const() {
  const auto next_tok = peek();
  if (is_next(token::TokenKind::True)) {
    const auto tok = expect(token::TokenKind::True);
  }
  else if (is_next(token::TokenKind::False)) {
    const auto tok = expect(token::TokenKind::False);
  }
  else {
    throw std::runtime_error("merkste selber wa");
  }
  const auto boolConstExpr = arena.create<BoolConstExpr>(next_tok.text, SourceLocation{
                                                                            lexer.get_file_name(), std::get<0>(next_tok.span.start),
                                                                            std::get<1>(next_tok.span.start)
                                                                        });
  return boolConstExpr;
}

Expression *Parser::parse_expr_tail(Expression *expr) {
  // TODO: ist das so optimal fieldId als VarExpr zu sehen?
  if (is_next(token::TokenKind::Dot)) {
    const auto dot = expect(token::TokenKind::Dot);
    const auto fieldId = expect(token::TokenKind::Identifier); // Field id
    const auto fieldExpr = arena.create<VarExpr>(fieldId->text,
                                                 SourceLocation{
                                                     lexer.get_file_name(), std::get<0>(fieldId->span.start),
                                                         std::get<1>(fieldId->span.start)
                                                             }
                                                 );
    const auto fieldAccess = arena.create<BinaryOperatorExpression>(
          expr, fieldExpr, BinaryOperator::FieldAccess, SourceLocation{
                                                          lexer.get_file_name(), std::get<0>(dot->span.start),
                                                          std::get<1>(dot->span.start)
                                                      }
        );
    return parse_expr_tail(fieldAccess);
  }
  else if (is_next(token::TokenKind::Arrow)) {
    const auto arrow = expect(token::TokenKind::Arrow);
    const auto fieldId = expect(token::TokenKind::Identifier); // Field id
    const auto fieldExpr = arena.create<VarExpr>(fieldId->text,
                                                 SourceLocation{
                                                     lexer.get_file_name(), std::get<0>(fieldId->span.start),
                                                     std::get<1>(fieldId->span.start)
                                                 }
    );
    const auto fieldAccess = arena.create<BinaryOperatorExpression>(
        expr, fieldExpr, BinaryOperator::PointerAccess, SourceLocation{
                                                          lexer.get_file_name(), std::get<0>(arrow->span.start),
                                                          std::get<1>(arrow->span.start)
                                                      }
    );
    return parse_expr_tail(fieldAccess);
  }
  else if (is_next(token::TokenKind::LBracket)) {
    const auto lbracket = expect(token::TokenKind::LBracket);
    Expression *index = parse_expression();
    const auto rbracket = expect(token::TokenKind::RBracket);
    const auto arrayAccess = arena.create<ArrayAccessExpr>(expr, index, SourceLocation{
                                                                            lexer.get_file_name(), std::get<0>(lbracket->span.start),
                                                                            std::get<1>(lbracket->span.start)
                                                                        });
    return parse_expr_tail(arrayAccess);
  }
  else if (is_next(token::TokenKind::Question)) {
    // TODO: add ternaryoperator to ast
    return expr;
  }
  else if (token::binary_ops.contains(peek().kind)) {
    const auto opToken = next_token();
    const auto binOp = binOpFromToken(opToken.kind);
    if (binOp == BinaryOperator::Unknown) {
      throw std::runtime_error("lol du bist cooked (token::binary_ops enthält ein token, "
                               "das in func binOpFromToken(...) nicht als BinaryOperator aufgeführt ist)");
    }
    const auto rightExpr = parse_expression();
    const auto binOpExpr = arena.create<BinaryOperatorExpression>(expr, rightExpr, binOp, SourceLocation{
                                                                                              lexer.get_file_name(),
                                                                                              std::get<0>(opToken.span.start),
                                                                                              std::get<1>(opToken.span.start)
                                                                                          });
    return parse_expr_tail(binOpExpr);
  }
  else {
    return expr;
  }
}

CallExpr *Parser::parse_call_expression() {
  const auto fn_name = expect(token::TokenKind::Identifier);
  const auto call_expr = arena.create<CallExpr>(fn_name->text);
  expect(token::TokenKind::LParen);
  if (!is_next(token::TokenKind::RParen)) {
    do {
      const auto param = parse_expression();
      call_expr->add_param(param);
    } while (match(token::TokenKind::Comma));
  }
  expect(token::TokenKind::RParen);
  return call_expr;
}

NumericExpr *Parser::parse_integer_literal() {
  const auto num = expect(token::TokenKind::NumberLiteral);
  const auto numExpr = arena.create<NumericExpr>(
      num->text,
      SourceLocation{lexer.get_file_name(), std::get<0>(num->span.start),
                     std::get<1>(num->span.start)});
  return numExpr;
}

CharLiteralExpr *Parser::parse_char_literal() {
  const auto ctok = expect(token::TokenKind::CharLiteral);
  const auto expr = arena.create<CharLiteralExpr>(ctok->text,
                                                  SourceLocation{lexer.get_file_name(), std::get<0>(ctok->span.start),
                                                                 std::get<1>(ctok->span.start)});
  return expr;
}

StringLiteralExpr *Parser::parse_string_literal() {
  const auto string = expect(token::TokenKind::StringLiteral);
  const auto expr = arena.create<StringLiteralExpr>(
      string->text,
      SourceLocation{lexer.get_file_name(), std::get<0>(string->span.start),
                     std::get<1>(string->span.start)});

  return expr;
}

Statement *Parser::parse_statement() {
  const auto next_token = peek();
  switch (next_token.kind) {
    case token::TokenKind::Return:
      return parse_return_statement();
    case token::TokenKind::Assert:
      return parse_assert_statement();
    case token::TokenKind::If:
      return parse_if_stmt();
    case token::TokenKind::While:
      return parse_while_stmt();
    case token::TokenKind::For:
      return parse_for_stmt();
    case token::TokenKind::Error:
      return parse_error_stmt();
    case token::TokenKind::LBrace: // {
      return parse_compound_statement();
    default:
      const auto stmt = parse_simple_stmt();
      expect(token::TokenKind::Semi);
      return stmt;
  }
}

bool Parser::is_var_decl_stmt() {
  const auto next_token = peek().kind;
  return next_token == token::TokenKind::Int || next_token == token::TokenKind::Bool || next_token == token::TokenKind::Void ||
         next_token == token::TokenKind::String || next_token == token::TokenKind::Char || next_token == token::TokenKind::Struct
        || check_sequence({token::TokenKind::Identifier, token::TokenKind::Identifier});
}

bool Parser::is_lv() {
  auto n_t = peek();
  size_t i = 1;
  while (n_t.kind != token::TokenKind::Eof && n_t.kind != token::TokenKind::Semi) {
    n_t = peek(i++);
    if (n_t.kind == token::TokenKind::PlusPlus || n_t.kind == token::TokenKind::MinusMinus
        || assmtOpFromToken(n_t.kind) != AssignmentOperator::Unknown) {
      return true;
    }
  }
  return false;
}


Statement *Parser::parse_simple_stmt() {
  if (is_var_decl_stmt()) {
    return parse_var_decl_stmt();
  }
  else if (is_lv()) {
    const auto lv = parse_lvalue();
    if (is_next(token::TokenKind::PlusPlus) || is_next(token::TokenKind::MinusMinus)) {
      const auto stmt = arena.create<UnaryMutationStatement>(lv,
        is_next(token::TokenKind::PlusPlus) ? UnaryMutationStatement::Op::PostIncrement : UnaryMutationStatement::Op::PostDecrement,
                                                             lv->get_location());
      return stmt;
    }
    else {
      const auto next_token = peek();
      const auto assmtOp = assmtOpFromToken(next_token.kind);
      if (assmtOp != AssignmentOperator::Unknown) {
        expect(next_token.kind);
        const auto expr = parse_expression();
        const auto stmt = arena.create<AssignmentStatement>(lv, assmtOp, expr,
                                                            lv->get_location());
        return stmt;
      }
      else {
        throw ParseError(std::format("Expected one of <simple> ::= \n| <lv> <asnop> <exp>\n| <lv> ++\n| <lv> --\n,"
                                     " but next token was {}", next_token.text));
      }
    }
  }
  // has to be expr
  else {
    const auto expr = parse_expression();
    const auto stmt = arena.create<ExpressionStatement>(expr, expr->get_location());
    return stmt;
  }
}

VariableDeclarationStatement *Parser::parse_var_decl_stmt() {
  printf("var-decl-stmt");
  Expression *expr = nullptr;
  const auto tp = parse_type();
  const auto ident = expect(token::TokenKind::Identifier);
  if (is_next(token::TokenKind::Equals)) {
    expect(token::TokenKind::Equals);
    expr = parse_expression();
  }
  const auto stmt = arena.create<VariableDeclarationStatement>(tp, ident->text, expr,
                                                               SourceLocation{lexer.get_file_name(),
                                                                              std::get<0>(ident->span.start),
                                                                              std::get<1>(ident->span.start)});
  return stmt;
}

ErrorStatement *Parser::parse_error_stmt() {
  const auto er = expect(token::TokenKind::Error);
  expect(token::TokenKind::LParen);
  const auto expr = parse_expression();
  expect(token::TokenKind::RParen);
  const auto stmt = arena.create<ErrorStatement>(expr,
                                                 SourceLocation{lexer.get_file_name(),
                                                                std::get<0>(er->span.start),
                                                                std::get<1>(er->span.start)});
  return stmt;
}

WhileStatement *Parser::parse_while_stmt() {
  const auto _while = expect(token::TokenKind::While);
  expect(token::TokenKind::LParen);
  const auto cond = parse_expression();
  expect(token::TokenKind::RParen);
  const auto body = parse_statement();
  const auto stmt = arena.create<WhileStatement>(cond, body,
                                                 SourceLocation{lexer.get_file_name(),
                                                                std::get<0>(_while->span.start),
                                                                std::get<1>(_while->span.start)});
  return stmt;
}

ForStatement *Parser::parse_for_stmt() {
  Statement *init = nullptr;
  Statement *incr = nullptr;
  const auto _for = expect(token::TokenKind::For);
  expect(token::TokenKind::LParen);
  if (!is_next(token::TokenKind::Semi)) {
    init = parse_statement();
  }
  expect(token::TokenKind::Semi);
  const auto cond = parse_expression();
  expect(token::TokenKind::Semi);
  if (!is_next(token::TokenKind::RParen)) {
    incr = parse_statement();
  }
  expect(token::TokenKind::RParen);
  const auto body = parse_statement();
  const auto stmt = arena.create<ForStatement>(init, cond, incr, body,
                                               SourceLocation{lexer.get_file_name(),
                                                              std::get<0>(_for->span.start),
                                                              std::get<1>(_for->span.start)});
  return stmt;
}

IfStatement *Parser::parse_if_stmt() {
  const auto _if = expect(token::TokenKind::If);
  expect(token::TokenKind::LParen);
  const auto cond = parse_expression();
  expect(token::TokenKind::RParen);
  const auto then = parse_statement();
  Statement *_else = nullptr;
  if (is_next(token::TokenKind::Else)) {
    expect(token::TokenKind::Else);
    _else = parse_statement();
  }
  const auto stmt = arena.create<IfStatement>(cond, then, _else,
                                              SourceLocation{lexer.get_file_name(), std::get<0>(_if->span.start),
                                                             std::get<1>(_if->span.start)});
  return stmt;
}

CompoundStmt *Parser::parse_compound_statement() {
  const auto l_brace = expect(token::TokenKind::LBrace);
  std::vector<Statement *> statements{};
  // TODO: no empty compounds allowed
  while (!check_sequence({token::TokenKind::RBrace})) {
    statements.push_back(parse_statement());
  }
  const auto r_brace = expect(token::TokenKind::RBrace);
  const auto compStmt = arena.create<CompoundStmt>(
      statements,
      SourceLocation{lexer.get_file_name(), std::get<0>(l_brace->span.start),
                     std::get<1>(r_brace->span.start)});
  return compStmt;
}
ReturnStmt *Parser::parse_return_statement() {
  const auto ret = expect(token::TokenKind::Return);
  const auto retStmt = arena.create<ReturnStmt>(
      SourceLocation{lexer.get_file_name(), std::get<0>(ret->span.start),
                     std::get<1>(ret->span.start)});
  if (!is_next(token::TokenKind::Semi)) {
    retStmt->set_expression(parse_expression());
  }
  expect(token::TokenKind::Semi);
  return retStmt;
}

AssertStmt *Parser::parse_assert_statement() {
  expect(token::TokenKind::Assert);
  expect(token::TokenKind::LParen);
  const auto expression = parse_expression();
  expect(token::TokenKind::RParen);

  auto assert_stmt = arena.create<AssertStmt>();
  assert_stmt->set_expression(expression);
  expect(token::TokenKind::Semi);
  return assert_stmt;
}

ParameterDeclaration *Parser::parse_parameter_declaration() {
  const auto param_type = parse_type();
  const auto ident{expect(token::TokenKind::Identifier)};
  const auto declaration = arena.create<ParameterDeclaration>(
      ident->text, param_type,
      SourceLocation{lexer.get_file_name(), std::get<0>(ident->span.start),
                     std::get<1>(ident->span.start)});
  return declaration;
}

FunctionDeclaration *Parser::parse_function_declaration() {
  const auto ret_type = parse_type();
  const auto ident{expect(token::TokenKind::Identifier)};
  expect(token::TokenKind::LParen);
  const auto declaration = arena.create<FunctionDeclaration>(
      ident->text, ret_type,
      SourceLocation{lexer.get_file_name(), std::get<0>(ident->span.start),
                     std::get<1>(ident->span.start)});
  if (!is_next(token::TokenKind::RParen)) {
    declaration->add_parameter_declaration(parse_parameter_declaration());
    while (check_sequence({token::TokenKind::Comma})) {
      expect(token::TokenKind::Comma);
      declaration->add_parameter_declaration(parse_parameter_declaration());
    }
  }
  expect(token::TokenKind::RParen);
  if (is_next(token::TokenKind::LBrace)) {
    declaration->set_body(parse_compound_statement());
  } else {
    expect(token::TokenKind::Semi);
  }

  return declaration;
}

TranslationUnit *Parser::parse_translation_unit() {
  auto *unit = arena.create<TranslationUnit>(
      SourceLocation{lexer.get_file_name(), 0, 0});
  while (!is_eof()) {
    try {
      unit->add_declaration(parse_function_declaration());
    } catch (ParseError &error) {
      spdlog::log(spdlog::level::err, error.what());
      synchronize();
    }
  }
  return unit;
}
