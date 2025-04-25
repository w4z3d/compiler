#ifndef TOKEN_H
#define TOKEN_H

#include <string>
namespace token {

enum class TokenType { STRING = 0 };

class Token {
  TokenType token_type;
  std::string literal;

public:
  TokenType get_token_type() const;

  Token(TokenType token_type);
  ~Token();
};

} // namespace token

#endif // TOKEN_H
