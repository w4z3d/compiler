#include "parser.hpp"
#include "spdlog/common.h"

FunctionDeclaration *Parser::parse_function_declaration() {
  const auto ret_type{expect(token::TokenKind::Identifier)};
  const auto ident{expect(token::TokenKind::Identifier)};
  expect(token::TokenKind::LParen);
  expect(token::TokenKind::RParen);
  expect(token::TokenKind::Semi);
  const auto declaration = arena.create<FunctionDeclaration>(
      ident->text, ret_type->text,
      SourceLocation{lexer.get_file_name(), std::get<0>(ident->span.start),
                     std::get<1>(ident->span.start)});
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
