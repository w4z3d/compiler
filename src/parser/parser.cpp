#include "parser.hpp"
#include "spdlog/common.h"

Expression *Parser::parse_expression() {
  const auto next_token = peek();

  switch (next_token.kind) {
  case token::TokenKind::Number:
    return parse_integer_literal();
  default:
    throw ParseError(std::format("Unexpected Token {}", next_token.text));
  }
}
NumericExpression *Parser::parse_integer_literal() {
  const auto num = expect(token::TokenKind::Number);
  const auto numExpr = arena.create<NumericExpression>(
      num->text,
      SourceLocation{lexer.get_file_name(), std::get<0>(num->span.start),
                     std::get<1>(num->span.start)});
  return numExpr;
}

Statement *Parser::parse_statement() {
  if (check_sequence({token::TokenKind::Return})) {
    return parse_return_statement();
  } else {
    // GRRRR
    expect(token::TokenKind::Return);
    return nullptr;
  }
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
  if (!check_sequence({token::TokenKind::Semi})) {
    // parse expression
    retStmt->setExpression(parse_integer_literal());
  }
  expect(token::TokenKind::Semi);
  return retStmt;
}

ParameterDeclaration *Parser::parse_parameter_declaration() {
  const auto param_type{expect(token::TokenKind::Identifier)};
  const auto ident{expect(token::TokenKind::Identifier)};
  const auto declaration = arena.create<ParameterDeclaration>(
      ident->text, param_type->text,
      SourceLocation{lexer.get_file_name(), std::get<0>(ident->span.start),
                     std::get<1>(ident->span.start)});
  return declaration;
}

FunctionDeclaration *Parser::parse_function_declaration() {
  const auto ret_type{expect(token::TokenKind::Identifier)};
  const auto ident{expect(token::TokenKind::Identifier)};
  expect(token::TokenKind::LParen);
  const auto declaration = arena.create<FunctionDeclaration>(
      ident->text, ret_type->text,
      SourceLocation{lexer.get_file_name(), std::get<0>(ident->span.start),
                     std::get<1>(ident->span.start)});

  if (check_sequence({token::TokenKind::Identifier})) {
    declaration->addParameterDeclaration(parse_parameter_declaration());
    while (check_sequence({token::TokenKind::Comma})) {
      expect(token::TokenKind::Comma);
      declaration->addParameterDeclaration(parse_parameter_declaration());
    }
  }
  expect(token::TokenKind::RParen);
  if (check_sequence({token::TokenKind::LBrace})) {
    declaration->setBody(parse_compound_statement());
  } else {
    expect(token::TokenKind::Semi);
  }

  return declaration;
}

TranslationUnit *Parser::parse_translation_unit() {
  TranslationUnit *unit = arena.create<TranslationUnit>(
      SourceLocation{lexer.get_file_name(), 0, 0});
  while (!is_eof()) {
    try {
      if (check_sequence({token::TokenKind::Identifier,
                          token::TokenKind::Identifier,
                          token::TokenKind::LParen})) {

        unit->addDeclaration(parse_function_declaration());
      } else {
        throw ParseError("Lmao wie dumm");
      }
    } catch (ParseError &error) {
      spdlog::log(spdlog::level::err, error.what());
      synchronize();
    }
  }
  return unit;
}
