#pragma once
#include <string>
#include <ostream>
#include <functional>

typedef signed int Literal;

struct Symbol
{
public:
  bool operator ==(Symbol other) const
  {
    return id == other.id;
  }

  const std::string &to_string(void) const;

private:
  Symbol(unsigned int id)
    : id(id)
  {}

private:
  const unsigned int id;

  template<typename T>
  friend class std::hash;
  friend class Token;
};

extern inline std::ostream &operator <<(std::ostream &os, Symbol sym)
{
  os << '`'
     << sym.to_string()
     << '`';
  return os;
}

namespace std
{

template<>
class hash<Symbol>
{
public:
  size_t operator ()(const Symbol &symbol) const
  {
    return std::hash<uint32_t>()(symbol.id);
  }
};

}
