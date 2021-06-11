#pragma once
#include <string>
#include <vector>
#include "token.h"

class Lexer
{
public:
  Lexer(std::string &&src)
    : src(src), tokens(), pos(0), loc(),
      cur_ch(this->src.length() > 0 ? this->src[0] : ' ')
  {}

  void lex_all(void);

private:
  char eof(void)
  {
    return pos >= src.length();
  }

  char peek(void)
  {
    return cur_ch;
  }

  char peek(off_t off)
  {
    if (pos + off >= src.length()
        || pos + off < 0)
      return EOF;
    return src[pos + off];
  }

  char bump(void)
  {
    char ch = cur_ch;
    if (ch == '\n')
      loc.newline();
    else
      loc.newchar();

    if (++pos < src.length())
      cur_ch = src[pos];
    else
      cur_ch = EOF;
    return ch;
  }

  [[ noreturn ]]
  void unexpected_char(void);
  [[ noreturn ]]
  void invalid_char_with_reason(const char *reason);

  void skip_singleline_comment(void);
  void skip_multiline_comment(void);

  Token lex_ident(void);
  Token lex_literal(void);
  Token lex_one(void);

private:
  const std::string src;
  std::vector<Token> tokens;
  size_t pos;
  Location loc;
  char cur_ch;

  friend class Parser;
};
