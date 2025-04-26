#include "token.hpp"

token::Token::Token(token::TokenType token_type)
    : token_type(token_type), literal("") {}

token::Token::~Token() = default;

token::TokenType token::Token::get_token_type() const { return token_type; }
