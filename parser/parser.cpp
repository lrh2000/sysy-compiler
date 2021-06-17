#include <iostream>
#include <algorithm>
#include <vector>
#include "../ast/ast.h"
#include "parser.h"

void Parser::expected_but_found(void)
{
  std::sort(expect_tok.rbegin(), expect_tok.rend());
  size_t sz = std::unique(expect_tok.begin(), expect_tok.end())
              - expect_tok.begin();

  switch (sz)
  {
  case 0:
    std::cerr << "internal compiler error: expected none, found "
              << *cur_tok
              << " "
              << cur_tok->location
              << std::endl;
    break;
  case 1:
    std::cerr << "error: expected "
              << Token::format(expect_tok[0])
              << ", found "
              << *cur_tok
              << " "
              << cur_tok->location
              << std::endl;
    break;
  case 2:
    std::cerr << "error: expected "
              << Token::format(expect_tok[0])
              << " or "
              << Token::format(expect_tok[1])
              << ", found "
              << *cur_tok
              << " "
              << cur_tok->location
              << std::endl;
    break;
  default:
    std::cerr << "error: expected one of "
              << sz
              << " tokens "
              << Token::format(expect_tok[0]);
    for (size_t i = 1; i < sz; ++i)
      std::cerr << " " << Token::format(expect_tok[i]);
    std::cerr << ", found "
              << *cur_tok
              << " "
              << cur_tok->location
              << std::endl;
    break;
  }
  abort();
}

std::vector<std::unique_ptr<AstExpr>> Parser::parse_indices(void)
{
  std::vector<std::unique_ptr<AstExpr>> indices;

  while (expect(Token::LeftSquare))
  {
    bump();
    auto expr = parse_expr();
    check_bump_or_err(Token::RightSquare);
    indices.emplace_back(std::move(expr));
  }

  return indices;
}

std::unique_ptr<AstLvalExpr> Parser::parse_lval_expr(Symbol name)
{
  auto indices = parse_indices();
  return std::make_unique<AstLvalExpr>(name, std::move(indices));
}

std::unique_ptr<AstCallExpr> Parser::parse_call_expr(Symbol name)
{
  std::vector<std::unique_ptr<AstExpr>> args;

  if (name.id == Token::StartTime
      || name.id == Token::StopTime) {
    args.emplace_back(
        std::make_unique<AstLiteralExpr>(bump().location.get_line()));
    check_bump_or_err(Token::RightRound);

    TokenKind tok;
    switch (name.id)
    {
    case Token::StartTime:
      tok = Token::SysyStartTime;
      break;
    case Token::StopTime:
      tok = Token::SysyStopTime;
      break;
    default:
      __builtin_unreachable();
    }

    return
      std::make_unique<AstCallExpr>(Symbol(tok), std::move(args));
  }

  bump();
  if (!expect(Token::RightRound)) {
    for (;;)
    {
      auto expr = parse_expr();
      args.emplace_back(std::move(expr));
      if (expect(Token::RightRound))
        break;
      else if (expect(Token::Comma))
        bump();
      else
        expected_but_found();
    }
  }
  bump();

  return std::make_unique<AstCallExpr>(name, std::move(args));
}

std::unique_ptr<AstExpr> Parser::parse_lval_or_call_expr(void)
{
  Symbol name = bump().to_symbol();
  if (expect(Token::LeftRound))
    return parse_call_expr(name);
  else
    return parse_lval_expr(name);
}

std::unique_ptr<AstLiteralExpr> Parser::parse_literal_expr(void)
{
  Literal literal = bump().literal;
  return std::make_unique<AstLiteralExpr>(literal);
}

std::unique_ptr<AstUnaryExpr> Parser::parse_unary_expr(void)
{
  AstUnaryOp op;
  switch (bump().kind)
  {
  case Token::Plus:
    op = AstUnaryOp::Pos;
    break;
  case Token::Minus:
    op = AstUnaryOp::Neg;
    break;
  case Token::Not:
    op = AstUnaryOp::Not;
    break;
  default:
    __builtin_unreachable();
  }

  auto expr = parse_primary_expr();
  return std::make_unique<AstUnaryExpr>(std::move(expr), op);
}

std::unique_ptr<AstExpr> Parser::parse_paren_expr(void)
{
  bump();
  auto expr = parse_expr();
  check_bump_or_err(Token::RightRound);
  return expr;
}

std::unique_ptr<AstExpr> Parser::parse_primary_expr(void)
{
  if (expect(Token::Plus) || expect(Token::Minus) || expect(Token::Not))
    return parse_unary_expr();
  else if (expect(Token::LeftRound))
    return parse_paren_expr();
  else if (expect(Token::Literal))
    return parse_literal_expr();
  else if (expect_ident())
    return parse_lval_or_call_expr();
  else
    expected_but_found();
}

std::unique_ptr<AstBinaryExpr>
Parser::parse_mul_expr(std::unique_ptr<AstExpr> &&lhs)
{
  AstBinaryOp op;

  switch (bump().kind)
  {
  case Token::Asterisk:
    op = AstBinaryOp::Mul;
    break;
  case Token::Slash:
    op = AstBinaryOp::Div;
    break;
  case Token::Percent:
    op = AstBinaryOp::Mod;
    break;
  default:
    __builtin_unreachable();
  }

  auto rhs = parse_primary_expr();
  return std::make_unique<AstBinaryExpr>(
      std::move(lhs), std::move(rhs), op);
}

std::unique_ptr<AstExpr> Parser::parse_mul_expr(void)
{
  std::unique_ptr<AstExpr> lhs = parse_primary_expr();

  while (expect(Token::Asterisk)
         || expect(Token::Slash)
         || expect(Token::Percent))
    lhs = parse_mul_expr(std::move(lhs));

  return lhs;
}

std::unique_ptr<AstBinaryExpr>
Parser::parse_add_expr(std::unique_ptr<AstExpr> &&lhs)
{
  AstBinaryOp op;

  switch (bump().kind)
  {
  case Token::Plus:
    op = AstBinaryOp::Add;
    break;
  case Token::Minus:
    op = AstBinaryOp::Sub;
    break;
  default:
    __builtin_unreachable();
  }

  auto rhs = parse_mul_expr();
  return std::make_unique<AstBinaryExpr>(
      std::move(lhs), std::move(rhs), op);
}

std::unique_ptr<AstExpr> Parser::parse_add_expr(void)
{
  std::unique_ptr<AstExpr> lhs = parse_mul_expr();

  while (expect(Token::Plus) || expect(Token::Minus))
    lhs = parse_add_expr(std::move(lhs));

  return lhs;
}

std::unique_ptr<AstExpr>
Parser::parse_expr_with_primary(std::unique_ptr<AstExpr> &&expr)
{
  std::unique_ptr lhs = std::move(expr);

  while (expect(Token::Asterisk) || expect(Token::Slash) || expect(Token::Percent))
    lhs = parse_mul_expr(std::move(lhs));

  while (expect(Token::Plus) || expect(Token::Minus))
    lhs = parse_add_expr(std::move(lhs));

  return lhs;
}

std::unique_ptr<AstExpr> Parser::parse_expr(void)
{
  return parse_add_expr();
}

std::unique_ptr<AstExprCond> Parser::parse_expr_cond(void)
{
  auto expr = parse_expr();
  return std::make_unique<AstExprCond>(std::move(expr));
}

std::unique_ptr<AstCond> Parser::parse_rel_cond(void)
{
  std::unique_ptr<AstCond> lhs = parse_expr_cond();

  for (;;)
  {
    AstLogicalOp op;
    if (expect(Token::Leq))
      op = AstLogicalOp::Leq;
    else if (expect(Token::Lt))
      op = AstLogicalOp::Lt;
    else if (expect(Token::Geq))
      op = AstLogicalOp::Geq;
    else if (expect(Token::Gt))
      op = AstLogicalOp::Gt;
    else
      break;
    bump();

    auto rhs = parse_expr_cond();
    lhs = std::make_unique<AstBinaryCond>(
          std::move(lhs), std::move(rhs), op);
  }

  return lhs;
}

std::unique_ptr<AstCond> Parser::parse_eq_cond(void)
{
  auto lhs = parse_rel_cond();

  for (;;)
  {
    AstLogicalOp op;
    if (expect(Token::Eq))
      op = AstLogicalOp::Eq;
    else if (expect(Token::Ne))
      op = AstLogicalOp::Ne;
    else
      break;
    bump();

    auto rhs = parse_rel_cond();
    lhs = std::make_unique<AstBinaryCond>(
          std::move(lhs), std::move(rhs), op);
  }

  return lhs;
}

std::unique_ptr<AstCond> Parser::parse_and_cond(void)
{
  auto lhs = parse_eq_cond();

  while (expect(Token::LogicalAnd))
  {
    bump();
    auto rhs = parse_eq_cond();
    lhs = std::make_unique<AstBinaryCond>(
          std::move(lhs), std::move(rhs), AstLogicalOp::And);
  }

  return lhs;
}

std::unique_ptr<AstCond> Parser::parse_or_cond(void)
{
  auto lhs = parse_and_cond();

  while (expect(Token::LogicalOr))
  {
    bump();
    auto rhs = parse_and_cond();
    lhs = std::make_unique<AstBinaryCond>(
          std::move(lhs), std::move(rhs), AstLogicalOp::Or);
  }
  
  return lhs;
}

std::unique_ptr<AstCond> Parser::parse_cond(void)
{
  return parse_or_cond();
}

std::unique_ptr<AstExprInit> Parser::parse_expr_init(void)
{
  auto expr = parse_expr();
  return std::make_unique<AstExprInit>(std::move(expr));
}

std::unique_ptr<AstListInit> Parser::parse_list_init(void)
{
  std::vector<std::unique_ptr<AstInit>> list;

  bump();
  if (!expect(Token::RightCurly)) {
    for (;;)
    {
      auto init = parse_init();
      list.emplace_back(std::move(init));
      if (expect(Token::RightCurly))
        break;
      else if (expect(Token::Comma))
        bump();
      else
        expected_but_found();
    }
  }
  bump();

  return std::make_unique<AstListInit>(std::move(list));
}

std::unique_ptr<AstInit> Parser::parse_init(void)
{
  if (expect(Token::LeftCurly))
    return parse_list_init();
  else
    return parse_expr_init();
}

std::unique_ptr<AstBlockStmt> Parser::parse_stmt_into_block(void)
{
  if (expect(Token::LeftCurly))
    return parse_block_stmt();

  auto stmt = parse_stmt();
  std::vector<std::unique_ptr<AstStmt>> stmts;
  stmts.emplace_back(std::move(stmt));
  return std::make_unique<AstBlockStmt>(std::move(stmts));
}

std::unique_ptr<AstDeclStmt> Parser::parse_decl_stmt(void)
{
  bool is_const = false;
  switch (bump().kind)
  {
  case Token::Const:
    is_const = true;
    check_bump_or_err(Token::Int);
    break;
  case Token::Int:
    break;
  default:
    __builtin_unreachable();
  }

  std::vector<Symbol> names;
  std::vector<std::vector<std::unique_ptr<AstExpr>>> indices;
  std::vector<std::unique_ptr<AstInit>> inits;

  for (;;)
  {
    names.emplace_back(check_bump_ident());
    indices.emplace_back(parse_indices());

    std::unique_ptr<AstInit> init = nullptr;
    if (expect(Token::Assign)) {
      bump();
      init = parse_init();
    } else if (is_const) {
      expected_but_found();
    }
    inits.emplace_back(std::move(init));

    if (expect(Token::Comma))
      bump();
    else
      break;
  }
  check_bump_or_err(Token::Semicolon);

  return std::make_unique<AstDeclStmt>(
        is_const, std::move(names),
        std::move(indices), std::move(inits));
}

std::unique_ptr<AstStmt> Parser::parse_if_stmt(void)
{
  bump();

  check_bump_or_err(Token::LeftRound);
  auto cond = parse_cond();
  check_bump_or_err(Token::RightRound);

  auto if_stmt = parse_stmt_into_block();
  if (!expect(Token::Else))
    return std::make_unique<AstIfStmt>(
          std::move(if_stmt), std::move(cond));
  bump();

  auto else_stmt = parse_stmt_into_block();
  return std::make_unique<AstIfElseStmt>(
        std::move(if_stmt), std::move(else_stmt), std::move(cond));
}

std::unique_ptr<AstWhileStmt> Parser::parse_while_stmt(void)
{
  bump();

  check_bump_or_err(Token::LeftRound);
  auto cond = parse_cond();
  check_bump_or_err(Token::RightRound);

  auto body = parse_stmt_into_block();
  return std::make_unique<AstWhileStmt>(
        std::move(body), std::move(cond));
}

std::unique_ptr<AstBreakStmt> Parser::parse_break_stmt(void)
{
  bump();
  check_bump_or_err(Token::Semicolon);
  return std::make_unique<AstBreakStmt>();
}

std::unique_ptr<AstContinueStmt> Parser::parse_continue_stmt(void)
{
  bump();
  check_bump_or_err(Token::Semicolon);
  return std::make_unique<AstContinueStmt>();
}

std::unique_ptr<AstReturnStmt> Parser::parse_return_stmt(void)
{
  bump();

  std::unique_ptr<AstExpr> expr = nullptr;
  if (!expect(Token::Semicolon)) {
    expr = parse_expr();
    if (!expect(Token::Semicolon))
      expected_but_found();
  }
  bump();

  return std::make_unique<AstReturnStmt>(std::move(expr));
}

std::unique_ptr<AstBlockStmt> Parser::parse_block_stmt(void)
{
  bump();

  std::vector<std::unique_ptr<AstStmt>> stmts;
  while (!expect(Token::RightCurly))
  {
    auto stmt = parse_stmt();
    stmts.emplace_back(std::move(stmt));
  }
  bump();

  return std::make_unique<AstBlockStmt>(std::move(stmts));
}

std::unique_ptr<AstEmptyStmt> Parser::parse_empty_stmt(void)
{
  bump();
  return std::make_unique<AstEmptyStmt>();
}

std::unique_ptr<AstExprStmt> Parser::parse_expr_stmt(void)
{
  auto expr = parse_expr();
  check_bump_or_err(Token::Semicolon);
  return std::make_unique<AstExprStmt>(std::move(expr));
}

std::unique_ptr<AstStmt> Parser::parse_expr_or_assign_stmt(void)
{
  Symbol name = bump().to_symbol();

  std::unique_ptr<AstExpr> lhs;
  if (!expect(Token::LeftRound)) {
    auto lval = parse_lval_expr(name);
    if (expect(Token::Assign)) {
      bump();
      auto rhs = parse_expr();
      check_bump_or_err(Token::Semicolon);
      return std::make_unique<AstAssignStmt>(
          std::move(lval), std::move(rhs));
    } else {
      lhs = std::move(lval);
    }
  } else {
    lhs = parse_call_expr(name);
  }

  auto expr = parse_expr_with_primary(std::move(lhs));
  check_bump_or_err(Token::Semicolon);
  return std::make_unique<AstExprStmt>(std::move(expr));
}

std::unique_ptr<AstStmt> Parser::parse_stmt(void)
{
  if (expect(Token::Int) || expect(Token::Const))
    return parse_decl_stmt();
  else if (expect(Token::If))
    return parse_if_stmt();
  else if (expect(Token::While))
    return parse_while_stmt();
  else if (expect(Token::Break))
    return parse_break_stmt();
  else if (expect(Token::Continue))
    return parse_continue_stmt();
  else if (expect(Token::Return))
    return parse_return_stmt();
  else if (expect(Token::LeftCurly))
    return parse_block_stmt();
  else if (expect(Token::Semicolon))
    return parse_empty_stmt();
  else if (expect_ident())
    return parse_expr_or_assign_stmt();
  else
    return parse_expr_stmt();
}

std::unique_ptr<AstFuncArg> Parser::parse_func_arg(void)
{
  check_bump_or_err(Token::Int);

  Symbol name = check_bump_ident();
  std::vector<std::unique_ptr<AstExpr>> indices;

  if (expect(Token::LeftSquare)) {
    bump();
    check_bump_or_err(Token::RightSquare);

    auto real_indices = parse_indices();
    indices.emplace_back(nullptr);
    for (auto &index : real_indices)
      indices.emplace_back(std::move(index));
  }

  return std::make_unique<AstFuncArg>(name, std::move(indices));
}

std::unique_ptr<AstDeclItem>
Parser::parse_decl_item(bool is_const, Symbol name)
{
  std::vector<Symbol> names;
  std::vector<std::vector<std::unique_ptr<AstExpr>>> indices;
  std::vector<std::unique_ptr<AstInit>> inits;

  names.emplace_back(name);
  for (;;)
  {
    indices.emplace_back(parse_indices());

    std::unique_ptr<AstInit> init = nullptr;
    if (expect(Token::Assign)) {
      bump();
      init = parse_init();
    } else if (is_const) {
      expected_but_found();
    }
    inits.emplace_back(std::move(init));

    if (expect(Token::Comma))
      bump();
    else
      break;
    names.emplace_back(check_bump_ident());
  }
  check_bump_or_err(Token::Semicolon);

  return std::make_unique<AstDeclItem>(
        is_const, std::move(names),
        std::move(indices), std::move(inits));
}

std::unique_ptr<AstFuncItem>
Parser::parse_func_item(bool is_void, Symbol name)
{
  bump();

  std::vector<std::unique_ptr<AstFuncArg>> args;
  if (!expect(Token::RightRound)) {
    for (;;)
    {
      auto arg = parse_func_arg();
      args.emplace_back(std::move(arg));
      if (expect(Token::Comma))
        bump();
      else if (expect(Token::RightRound))
        break;
      else
        expected_but_found();
    }
  }
  bump();

  check_bump_or_err(Token::LeftCurly);
  std::vector<std::unique_ptr<AstStmt>> body;
  while (!expect(Token::RightCurly))
  {
    auto stmt = parse_stmt();
    body.emplace_back(std::move(stmt));
  }
  bump();

  return std::make_unique<AstFuncItem>(
        is_void, name, std::move(args), std::move(body));
}

std::unique_ptr<AstItem> Parser::parse_item(void)
{
  bool has_const = false;
  if (expect(Token::Const)) {
    has_const = true;
    bump();
  }

  bool is_void = false;
  if (expect(Token::Int)) {
    bump();
  } else if (!has_const && expect(Token::Void)) {
    is_void = true;
    bump();
  } else {
    expected_but_found();
  }

  Symbol name = check_bump_ident();

  if (!has_const && expect(Token::LeftRound))
    return parse_func_item(is_void, name);
  else if (!is_void)
    return parse_decl_item(has_const, name);
  else
    expected_but_found();
}

std::unique_ptr<AstCompUnit> Parser::parse_comp_unit(void)
{
  std::vector<std::unique_ptr<AstItem>> items;

  while (!expect(Token::Eof))
  {
    auto item = parse_item();
    items.emplace_back(std::move(item));
  }

  return std::make_unique<AstCompUnit>(std::move(items));
}
