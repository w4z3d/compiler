#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType { STRING = 0 };

class Token {
public:
  TokenType token_type;
  std::string literal;
  Token(TokenType token_type);
  ~Token();
};

#endif // TOKEN_H
