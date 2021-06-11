#include <string>
#include "defid.h"
#include "../lexer/token.h"

Symbol AstDefId::make_unique_symbol(void) const
{
  std::string str = "__"
    + std::to_string(scope)
    + "_"
    + std::to_string(id);

  return Token(str, Location()).to_symbol();
}
