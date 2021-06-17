#include <iostream>
#include "ast.h"
#include "context.h"
#include "../lexer/token.h"

void AstBinaryExpr::name_resolve(AstContext *ctx)
{
  lhs->name_resolve(ctx);
  rhs->name_resolve(ctx);
}

void AstUnaryExpr::name_resolve(AstContext *ctx)
{
  expr->name_resolve(ctx);
}

void AstLvalExpr::name_resolve(AstContext *ctx)
{
  ref = ctx->def_lookup(sym);
  if (!ref.is_valid()) {
    std::cerr << "error: use of undeclared variable "
              << sym
              << std::endl;
    abort();
  }

  for (auto &index : indices)
    index->name_resolve(ctx);
}

void AstLiteralExpr::name_resolve(AstContext *ctx)
{ /* nothing */ }

void AstCallExpr::name_resolve(AstContext *ctx)
{
  ref = ctx->def_lookup(sym);
  if (!ref.is_valid()) {
    std::cerr << "error: call of undeclared function "
              << sym
              << std::endl;
    abort();
  }

  for (auto &arg : args)
    arg->name_resolve(ctx);
}

void AstExprCond::name_resolve(AstContext *ctx)
{
  expr->name_resolve(ctx);
}

void AstBinaryCond::name_resolve(AstContext *ctx)
{
  lhs->name_resolve(ctx);
  rhs->name_resolve(ctx);
}

void AstExprInit::name_resolve(AstContext *ctx)
{
  expr->name_resolve(ctx);
}

void AstListInit::name_resolve(AstContext *ctx)
{
  for (auto &elem : list)
    elem->name_resolve(ctx);
}

void AstExprStmt::name_resolve(AstContext *ctx)
{
  expr->name_resolve(ctx);
}

void AstReturnStmt::name_resolve(AstContext *ctx)
{
  if (expr != nullptr)
    expr->name_resolve(ctx);
}

void AstDeclStmt::name_resolve(AstContext *ctx)
{
  for (size_t i = 0; i < sym.size(); ++i)
  {
    for (auto &index : indices[i])
      index->name_resolve(ctx);

    def.emplace_back(ctx->def_insert(sym[i]));
    if (!def.back().is_valid()) {
      std::cerr << "error: redefinition of variable "
                << sym[i]
                << std::endl;
      abort();
    }

    if (init[i] != nullptr)
      init[i]->name_resolve(ctx);
  }
}

void AstAssignStmt::name_resolve(AstContext *ctx)
{
  lhs->name_resolve(ctx);
  rhs->name_resolve(ctx);
}

void AstBlockStmt::name_resolve(AstContext *ctx)
{
  ctx->scope_push();

  for (auto &stmt: stmts)
    stmt->name_resolve(ctx);

  ctx->scope_pop();
}

void AstIfStmt::name_resolve(AstContext *ctx)
{
  cond->name_resolve(ctx);
  if_stmt->name_resolve(ctx);
}

void AstIfElseStmt::name_resolve(AstContext *ctx)
{
  cond->name_resolve(ctx);
  if_stmt->name_resolve(ctx);
  else_stmt->name_resolve(ctx);
}

void AstWhileStmt::name_resolve(AstContext *ctx)
{
  ctx->loop_push();

  cond->name_resolve(ctx);
  body->name_resolve(ctx);

  ctx->loop_pop();
}

void AstBreakStmt::name_resolve(AstContext *ctx)
{
  if (!ctx->loop_check()) {
    std::cerr << "error: cannot `break` outside loops"
              << std::endl;
    abort();
  }
}

void AstContinueStmt::name_resolve(AstContext *ctx)
{
  if (!ctx->loop_check()) {
    std::cerr << "error: cannot `continue` outside loops"
              << std::endl;
    abort();
  }
}

void AstEmptyStmt::name_resolve(AstContext *ctx)
{ /* nothing */ }

void AstFuncArg::name_resolve(AstContext *ctx)
{
  if (!(def = ctx->def_insert(sym)).is_valid()) {
    std::cerr << "error: redefinition of variable "
              << sym
              << std::endl;
    abort();
  }

  for (auto &index : indices)
    if (index != nullptr)
      index->name_resolve(ctx);
}

void AstFuncItem::name_resolve(AstContext *ctx)
{
  if (!(def = ctx->def_insert(sym)).is_valid()) {
    std::cerr << "error: redefinition of function "
              << sym
              << std::endl;
    abort();
  }

  if (args.size() > 8) {
    std::cerr << "error: too many arguments of function "
              << sym
              << std::endl;
    abort();
  }

  ctx->scope_push();

  for (auto &arg : args)
    arg->name_resolve(ctx);

  for (auto &stmt : body)
    stmt->name_resolve(ctx);

  ctx->scope_pop();
}

void AstDeclItem::name_resolve(AstContext *ctx)
{
  for (size_t i = 0; i < sym.size(); ++i)
  {
    for (auto &index : indices[i])
      index->name_resolve(ctx);

    def.emplace_back(ctx->def_insert(sym[i]));
    if (!def.back().is_valid()) {
      std::cerr << "redefinition of variable "
                << sym[i]
                << std::endl;
      abort();
    }

    if (init[i] != nullptr)
      init[i]->name_resolve(ctx);
  }
}

void AstCompUnit::name_resolve(AstContext *ctx)
{
  ctx->scope_push();

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::GetInt)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::PutInt)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::GetCh)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::PutCh)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::GetArray)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::PutArray)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::SysyStartTime)));
  assert(prelude_defs.back().is_valid());

  prelude_defs.emplace_back(
      ctx->def_insert(Symbol(Token::SysyStopTime)));
  assert(prelude_defs.back().is_valid());

  for (auto &item : items)
    item->name_resolve(ctx);

  ctx->scope_pop();
}
