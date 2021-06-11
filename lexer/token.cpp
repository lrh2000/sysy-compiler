#include <vector>
#include <unordered_map>
#include "token.h"

static std::vector<std::string> sym_vec = {
#define LIST_TOKEN(str, name) \
  str,
  LIST_TOKENS
#undef LIST_TOKEN
};

static std::unordered_map<std::string, TokenKind> sym_map = {
#define LIST_TOKEN(str, name) \
  {str, Token::name},
  LIST_TOKENS
#undef LIST_TOKEN
};

static TokenKind intern_string(const std::string &str)
{
  auto it = sym_map.find(str);
  if (it == sym_map.end()) {
    sym_map[str] = sym_vec.size();
    sym_vec.emplace_back(str);
    return sym_vec.size() - 1;
  }
  return it->second;
}

Token::Token(const std::string &str, Location location)
  : kind(intern_string(str)), literal(0), location(location)
{
  assert(is_keyword_or_ident());
}

std::string Token::format(TokenKind kind)
{
  if (kind >= Ident)
    return "{identifier}";
  else if (kind == Literal)
    return "{number}";
  else if (kind == Eof)
    return "{end of file}";
  else
    return '`' + sym_vec[kind] + '`';
}

const std::string &Symbol::to_string(void) const
{
  return sym_vec[id];
}
