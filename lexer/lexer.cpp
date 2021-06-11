#include <cctype>
#include <cassert>
#include <iostream>
#include "token.h"
#include "lexer.h"

void Lexer::unexpected_char(void)
{
  if (eof()) {
    std::cerr << "error: unexpected end of file "
              << loc
              << std::endl;
  } else if (peek() == '\n') {
    std::cerr << "error: unexpected new line "
              << loc
              << std::endl;
  } else {
    std::cerr << "error: unexpected character '"
              << peek()
              << "' "
              << loc
              << std::endl;
  }
  abort();
}

void Lexer::invalid_char_with_reason(const char *reason)
{
  if (eof()) {
    std::cerr << "error: unexpected end of file "
              << reason
              << loc
              << std::endl;
  } else if (peek() == '\n') {
    std::cerr << "error: unexpected new line "
              << reason
              << loc
              << std::endl;
  } else {
    std::cerr << "error: invalid character '"
              << peek()
              << "' "
              << reason
              << loc
              << std::endl;
  }
  abort();
}



void Lexer::skip_singleline_comment(void)
{
  while (!eof() && peek() != '\n')
    bump();
}

void Lexer::skip_multiline_comment(void)
{
  while (!eof())
  {
    if (peek() == '*') {
      bump();
      if (peek() != '/')
        continue;
      bump();
      return;
    }
    bump();
  }
  std::cerr << "error: unterminated multi-line comment "
            << loc
            << std::endl;
  abort();
}

Token Lexer::lex_literal(void)
{
  int base;
  Location location = loc;

  assert(isdigit(peek()));
  if (peek() == '0') {
    switch (peek(1))
    {
    case 'x':
    case 'X':
      bump();
      bump();
      base = 16;
      break;
    default:
      base = 8;
      break;
    }
  } else {
    base = 10;
  }

  bool ok = false;
  Literal literal = 0;
  while (!eof())
  {
    char ch = peek();
    bool err = false;
    int digit;

    if (ch >= '0' && ch <= '9')
      digit = ch - '0';
    else if (ch >= 'a' && ch <= 'f')
      digit = ch - 'a' + 10;
    else if (ch >= 'A' && ch <= 'F')
      digit = ch - 'A' + 10;
    else if (isalpha(ch))
      err = true;
    else if (!ok)
      err = true;
    else
      break;

    if (digit >= base)
      err = true;
    if (err)
      invalid_char_with_reason("on integer constant ");

    literal = literal * base + digit;
    bump();
    ok = true;
  }
  if (!ok)
    invalid_char_with_reason("on integer constant ");

  return Token(Token::Literal, location, literal);
}

Token Lexer::lex_ident(void)
{
  size_t start = pos;
  Location location = loc;

  assert(isalpha(peek()));
  do {
    bump();
  } while (!eof() && (isdigit(peek()) || isalpha(peek())));

  return Token(src.substr(start, pos - start), location);
}

Token Lexer::lex_one(void)
{
retry:
  while (!eof() && isspace(peek()))
    bump();
  if (eof())
    return Token(Token::Eof, loc);

  TokenKind kind;
  Location location = loc;
  switch (char c = peek())
  {
  case ';':
    kind = Token::Semicolon;
    break;
  case ',':
    kind = Token::Comma;
    break;
  case '+':
    kind = Token::Plus;
    break;
  case '-':
    kind = Token::Minus;
    break;
  case '*':
    kind = Token::Asterisk;
    break;
  case '/':
    switch (peek(1))
    {
    case '/':
      bump();
      bump();
      skip_singleline_comment();
      goto retry;
    case '*':
      bump();
      bump();
      skip_multiline_comment();
      goto retry;
    default:
      kind = Token::Slash;
      break;
    }
    break;
  case '%':
    kind = Token::Percent;
    break;
  case '=':
    if (peek(1) == '=') {
      kind = Token::Eq;
      bump();
    } else {
      kind = Token::Assign;
    }
    break;
  case '<':
    switch (peek(1))
    {
    case '<':
      kind = Token::LeftShift;
      bump();
      break;
    case '=':
      kind = Token::Leq;
      bump();
      break;
    default:
      kind = Token::Lt;
      break;
    }
    break;
  case '>':
    switch (peek(1))
    {
    case '>':
      kind = Token::RightShift;
      bump();
      break;
    case '=':
      kind = Token::Geq;
      bump();
      break;
    default:
      kind = Token::Gt;
      break;
    }
    break;
  case '&':
    if (peek(1) == '&') {
      kind = Token::LogicalAnd;
      bump();
    } else {
      std::cerr << "error: unsupported token `&`, did you mean `&&`? "
                << loc
                << std::endl;
      abort();
    }
    break;
  case '|':
    if (peek(1) == '|') {
      kind = Token::LogicalOr;
      bump();
    } else {
      std::cerr << "error: unsupported token `|`, did you mean `||`? "
                << loc
                << std::endl;
      abort();
    }
    break;
  case '!':
    if (peek(1) == '=') {
      kind = Token::Ne;
      bump();
    } else {
      kind = Token::Not;
    }
    break;
  case '(':
    kind = Token::LeftRound;
    break;
  case ')':
    kind = Token::RightRound;
    break;
  case '[':
    kind = Token::LeftSquare;
    break;
  case ']':
    kind = Token::RightSquare;
    break;
  case '{':
    kind = Token::LeftCurly;
    break;
  case '}':
    kind = Token::RightCurly;
    break;
  default:
    if (isalpha(c))
      return lex_ident();
    else if (isdigit(c))
      return lex_literal();
    else
      unexpected_char();
  }
  bump();

  return Token(kind, location);
}

void Lexer::lex_all(void)
{
  for (;;)
  {
    Token token = lex_one();
    tokens.emplace_back(token);
    if (token.kind == Token::Eof)
      break;
  }
}
