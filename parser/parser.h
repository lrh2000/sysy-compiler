#pragma once
#include <vector>
#include <memory>
#include "../lexer/token.h"
#include "../lexer/lexer.h"

class AstExpr;

class AstLvalExpr;
class AstCallExpr;
class AstLiteralExpr;
class AstUnaryExpr;
class AstBinaryExpr;

class AstCond;
class AstExprCond;

class AstExprInit;
class AstListInit;
class AstInit;

class AstStmt;
class AstBlockStmt;
class AstDeclStmt;
class AstWhileStmt;
class AstBreakStmt;
class AstContinueStmt;
class AstReturnStmt;
class AstBlockStmt;
class AstEmptyStmt;
class AstExprStmt;

class AstFuncArg;

class AstItem;
class AstDeclItem;
class AstFuncItem;

class AstCompUnit;

class Parser
{
public:
  Parser(Lexer &&lexer)
    : tokens(std::move(lexer.tokens)),
      pos(0),
      cur_tok(&this->tokens[0]),
      expect_tok()
  {}

  std::unique_ptr<AstCompUnit> parse_comp_unit(void);

private:
  std::vector<std::unique_ptr<AstExpr>> parse_indices(void);

  std::unique_ptr<AstLvalExpr> parse_lval_expr(Symbol name);
  std::unique_ptr<AstCallExpr> parse_call_expr(Symbol name);
  std::unique_ptr<AstExpr> parse_lval_or_call_expr(void);
  std::unique_ptr<AstLiteralExpr> parse_literal_expr(void);
  std::unique_ptr<AstUnaryExpr> parse_unary_expr(void);
  std::unique_ptr<AstExpr> parse_paren_expr(void);
  std::unique_ptr<AstExpr> parse_primary_expr(void);
  std::unique_ptr<AstBinaryExpr>
  parse_mul_expr(std::unique_ptr<AstExpr> &&lhs);
  std::unique_ptr<AstExpr> parse_mul_expr(void);
  std::unique_ptr<AstBinaryExpr>
  parse_add_expr(std::unique_ptr<AstExpr> &&lhs);
  std::unique_ptr<AstExpr> parse_add_expr(void);
  std::unique_ptr<AstExpr>
  parse_expr_with_primary(std::unique_ptr<AstExpr> &&expr);
  std::unique_ptr<AstExpr> parse_expr(void);

  std::unique_ptr<AstExprCond> parse_expr_cond(void);
  std::unique_ptr<AstCond> parse_rel_cond(void);
  std::unique_ptr<AstCond> parse_eq_cond(void);
  std::unique_ptr<AstCond> parse_and_cond(void);
  std::unique_ptr<AstCond> parse_or_cond(void);
  std::unique_ptr<AstCond> parse_cond(void);

  std::unique_ptr<AstExprInit> parse_expr_init(void);
  std::unique_ptr<AstListInit> parse_list_init(void);
  std::unique_ptr<AstInit> parse_init(void);

  std::unique_ptr<AstBlockStmt> parse_stmt_into_block(void);
  std::unique_ptr<AstDeclStmt> parse_decl_stmt(void);
  std::unique_ptr<AstStmt> parse_if_stmt(void);
  std::unique_ptr<AstWhileStmt> parse_while_stmt(void);
  std::unique_ptr<AstBreakStmt> parse_break_stmt(void);
  std::unique_ptr<AstContinueStmt> parse_continue_stmt(void);
  std::unique_ptr<AstReturnStmt> parse_return_stmt(void);
  std::unique_ptr<AstBlockStmt> parse_block_stmt(void);
  std::unique_ptr<AstEmptyStmt> parse_empty_stmt(void);
  std::unique_ptr<AstExprStmt> parse_expr_stmt(void);
  std::unique_ptr<AstStmt> parse_expr_or_assign_stmt(void);
  std::unique_ptr<AstStmt> parse_stmt(void);

  std::unique_ptr<AstFuncArg> parse_func_arg(void);

  std::unique_ptr<AstDeclItem> parse_decl_item(bool is_const, Symbol name);
  std::unique_ptr<AstFuncItem> parse_func_item(bool is_void, Symbol name);
  std::unique_ptr<AstItem> parse_item(void);

private:
  Token bump(void)
  {
    Token tok = *cur_tok;
    if (++pos < tokens.size())
      cur_tok = &tokens[pos];
    return tok;
  }

  bool expect(TokenKind kind)
  {
    if (cur_tok->kind == kind) {
      expect_tok.clear();
      return true;
    } else {
      expect_tok.emplace_back(kind);
      return false;
    }
  }

  bool expect_ident(void)
  {
    if (cur_tok->kind >= Token::Ident) {
      expect_tok.clear();
      return true;
    } else {
      expect_tok.emplace_back(Token::Ident);
      return false;
    }
  }

  [[ noreturn ]]
  void expected_but_found(void);

  Token check_bump_or_err(TokenKind kind)
  {
    if (expect(kind))
      return bump();
    expected_but_found();
  }

  Symbol check_bump_ident(void)
  {
    if (expect_ident())
      return bump().to_symbol();
    expected_but_found();
  }

private:
  const std::vector<Token> tokens;
  size_t pos;
  const Token *cur_tok;
  std::vector<TokenKind> expect_tok;
};
