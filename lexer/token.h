#pragma once
#include <cstddef>
#include <cassert>
#include <string>
#include <ostream>
#include <functional>
#include "symbol.h"

#define LIST_TOKENS                        \
  LIST_TOKEN("", Eof)                      \
                                           \
  LIST_TOKEN(";", Semicolon)               \
  LIST_TOKEN(",", Comma)                   \
  LIST_TOKEN("+", Plus)                    \
  LIST_TOKEN("-", Minus)                   \
  LIST_TOKEN("*", Asterisk)                \
  LIST_TOKEN("/", Slash)                   \
  LIST_TOKEN("%", Percent)                 \
  LIST_TOKEN("=", Assign)                  \
  LIST_TOKEN("<", Lt)                      \
  LIST_TOKEN(">", Gt)                      \
  LIST_TOKEN("!", Not)                     \
  LIST_TOKEN("(", LeftRound)               \
  LIST_TOKEN(")", RightRound)              \
  LIST_TOKEN("[", LeftSquare)              \
  LIST_TOKEN("]", RightSquare)             \
  LIST_TOKEN("{", LeftCurly)               \
  LIST_TOKEN("}", RightCurly)              \
                                           \
  LIST_TOKEN("<=", Leq)                    \
  LIST_TOKEN(">=", Geq)                    \
  LIST_TOKEN("==", Eq)                     \
  LIST_TOKEN("!=", Ne)                     \
  LIST_TOKEN("<<", LeftShift)              \
  LIST_TOKEN(">>", RightShift)             \
  LIST_TOKEN("&&", LogicalAnd)             \
  LIST_TOKEN("||", LogicalOr)              \
                                           \
  LIST_TOKEN("0", Literal)                 \
                                           \
  LIST_TOKEN("void", Void)                 \
  LIST_TOKEN("int", Int)                   \
  LIST_TOKEN("const", Const)               \
  LIST_TOKEN("if", If)                     \
  LIST_TOKEN("else", Else)                 \
  LIST_TOKEN("while", While)               \
  LIST_TOKEN("break", Break)               \
  LIST_TOKEN("continue", Continue)         \
  LIST_TOKEN("return", Return)             \
                                           \
  LIST_TOKEN("getint", GetInt)             \
  LIST_TOKEN("putint", PutInt)             \
  LIST_TOKEN("getch", GetCh)               \
  LIST_TOKEN("putch", PutCh)               \
  LIST_TOKEN("getarray", GetArray)         \
  LIST_TOKEN("putarray", PutArray)         \
  LIST_TOKEN("starttime", StartTime)       \
  LIST_TOKEN("stoptime", StopTime)               \
  LIST_TOKEN("_sysy_starttime", SysyStartTime)  \
  LIST_TOKEN("_sysy_stoptime", SysyStopTime)

typedef unsigned int TokenKind;

struct Location
{
public:
  Location(void)
    : line(1), column(1)
  {}

  void newchar(void)
  {
    ++column;
  }

  void newline(void)
  {
    column = 1;
    ++line;
  }

  unsigned int get_line(void) const
  {
    return line;
  }

  unsigned int get_column(void) const
  {
    return column;
  }

private:
  unsigned int line;
  unsigned int column;
};

extern inline std::ostream &operator <<(std::ostream &os, Location loc)
{
  os << "(line "
     << loc.get_line()
     << ", column "
     << loc.get_column()
     << ")";
  return os;
}

struct Token
{
  enum
  {
#define LIST_TOKEN(str, name) \
    name,
    LIST_TOKENS
#undef LIST_TOKEN

    Ident = GetInt,
  };

  const TokenKind kind;
  const ::Literal literal;
  const Location location;

public:
  static std::string format(TokenKind kind);

  Symbol to_symbol(void) const
  {
    assert(is_ident());
    return Symbol(kind);
  }

private:
  Token(TokenKind kind, Location location, ::Literal literal = 0)
    : kind(kind), literal(literal), location(location)
  {}

  Token(const std::string &str, Location location);

  bool is_keyword(void) const
  {
    return kind >= Void && kind < Ident;
  }

  bool is_ident(void) const
  {
    return kind >= Ident;
  }

  bool is_keyword_or_ident(void) const
  {
    return kind >= Void;
  }

  friend class Lexer;
  friend class AstDefId;
};

extern inline std::ostream &operator <<(std::ostream &os, const Token &token)
{
  os << Token::format(token.kind);
  return os;
}
