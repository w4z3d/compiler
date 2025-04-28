#include "parser.hpp"

FunctionDeclaration *Parser::parse_function_declaration() {
  const auto ret_type{expect(token::TokenKind::Identifier)};
  const auto ident{expect(token::TokenKind::Identifier)};
  expect(token::TokenKind::LParen);
  expect(token::TokenKind::RParen);
  expect(token::TokenKind::Semi);
  const auto declaration =
      arena.create<FunctionDeclaration>(ident->text, ret_type->text);
  return declaration;
}

TranslationUnit *Parser::parse_translation_unit() {
  TranslationUnit *unit = arena.create<TranslationUnit>(
      SourceLocation{lexer.get_file_name(), 0, 0});
  while (!is_eof()) {
    if (check_sequence({token::TokenKind::Identifier,
                        token::TokenKind::Identifier,
                        token::TokenKind::LParen})) {

      unit->addDeclaration(parse_function_declaration());
    }

    next_token();
  }
  return unit;
}
