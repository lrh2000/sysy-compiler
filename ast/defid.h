#pragma once
#include <cstddef>
#include <functional>
#include "../lexer/symbol.h"

typedef unsigned int AstScopeId;
typedef unsigned int AstLocalDefId;

struct AstDefId
{
public:
  AstDefId(void)
    : scope(~0u), id(~0u)
  {}

  bool is_valid(void) const
  {
    return ~scope && ~id;
  }

  Symbol make_unique_symbol(void) const;

  bool is_global(void) const
  {
    return scope == 0;
  }

private:
  AstDefId(AstScopeId scope, AstLocalDefId id)
    : scope(scope), id(id)
  {}

  AstScopeId scope;
  AstLocalDefId id;

  template<typename T>
  friend class std::hash;
  friend class AstContext;
};

namespace std
{

template<>
class hash<AstDefId>
{
public:
  size_t operator ()(const AstDefId &defid) const
  {
    uint64_t value;
    value = defid.scope;
    value <<= 32;
    value |= defid.id;
    return std::hash<uint64_t>()(value);
  }
};

}
