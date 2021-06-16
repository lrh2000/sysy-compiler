#include "ast.h"
#include "type.h"
#include "context.h"
#include "../hir/hir.h"
#include "../hir/builder.h"

std::unique_ptr<AstExpr> AstExpr::take_if_unary_not(void)
{
  return nullptr;
}

std::unique_ptr<AstExpr> AstUnaryExpr::take_if_unary_not(void)
{
  if (op == AstUnaryOp::Not)
    return std::move(expr);
  return nullptr;
}

std::unique_ptr<HirExpr> AstBinaryExpr::translate(AstContext *ctx)
{
  HirBinaryOp hir_op;
  switch (op)
  {
  case AstBinaryOp::Add:
    hir_op = HirBinaryOp::Add;
    break;
  case AstBinaryOp::Sub:
    hir_op = HirBinaryOp::Sub;
    break;
  case AstBinaryOp::Mul:
    hir_op = HirBinaryOp::Mul;
    break;
  case AstBinaryOp::Div:
    hir_op = HirBinaryOp::Div;
    break;
  case AstBinaryOp::Mod:
    hir_op = HirBinaryOp::Mod;
    break;
  }

  auto hir_lhs = lhs->translate(ctx);
  auto hir_rhs = rhs->translate(ctx);
  return std::make_unique<HirBinaryExpr>(hir_op,
      std::move(hir_lhs), std::move(hir_rhs));
}

std::unique_ptr<HirExpr> AstUnaryExpr::translate(AstContext *ctx)
{
  HirUnaryOp hir_op;
  switch (op)
  {
  case AstUnaryOp::Not:
    hir_op = HirUnaryOp::Not;
    break;
  case AstUnaryOp::Neg:
    hir_op = HirUnaryOp::Neg;
    break;
  case AstUnaryOp::Pos:
    return expr->translate(ctx);
  }

  auto hir_expr = expr->translate(ctx);
  return std::make_unique<HirUnaryExpr>(hir_op, std::move(hir_expr));
}

std::unique_ptr<HirExpr> AstLvalExpr::into_addr(AstContext *ctx)
{
  auto ty_base = ctx->def_get_type(ref);

  if (ty_base->get_kind() == AstTypeKind::Array
      || ty_base->get_kind() == AstTypeKind::Ptr) {
    std::unique_ptr<HirExpr> base;
    const std::vector<unsigned int> *pshape;
    bool is_ptr;
    switch (ty_base->get_kind())
    {
    case AstTypeKind::Array:
      if (auto ty = static_cast<const AstArrayType *>(ty_base);
          !ty->is_const() && !ref.is_global()) {
        base =
          std::make_unique<HirLocalAddrExpr>(ctx->def_get_arrayid(ref), 0);
        pshape = &ty->get_shape();
      } else {
        base =
          std::make_unique<HirGlobalAddrExpr>(ctx->def_get_symbol(ref), 0);
        pshape = &ty->get_shape();
      }
      is_ptr = false;
      break;
    case AstTypeKind::Ptr:
      assert(!ref.is_global());
      base = std::make_unique<HirLocalVarExpr>(ctx->def_get_localid(ref));
      pshape = &static_cast<const AstPtrType *>(ty_base)->get_shape();
      is_ptr = true;
      break;
    default:
      __builtin_unreachable();
    }
    const auto &shape = *pshape;
    assert(indices.size() <= shape.size() + is_ptr);

    if (indices.size() == 0)
      return base;

    size_t strip = sizeof(int);
    for (size_t i = shape.size() + is_ptr - 1; i >= indices.size(); --i)
      strip *= shape[i - is_ptr];
    for (size_t i = indices.size() - 1; ; --i)
    {
      auto literal = std::make_unique<HirLiteralExpr>(strip);
      auto hir_arg = indices[i]->translate(ctx);
      auto offset = std::make_unique<HirBinaryExpr>(
          HirBinaryOp::Mul, std::move(hir_arg), std::move(literal));
      base = std::make_unique<HirBinaryExpr>(
          HirBinaryOp::Add, std::move(base), std::move(offset));
      if (i != 0)
        strip *= shape[i - is_ptr];
      else
        break;
    }

    return base;
  } else if (ty_base->get_kind() == AstTypeKind::Int) {
    assert(ref.is_global());
    assert(indices.size() == 0);
    return
      std::make_unique<HirGlobalAddrExpr>(ctx->def_get_symbol(ref), 0);
  } else {
    abort();
  }
}

std::unique_ptr<HirExpr> AstLvalExpr::translate(AstContext *ctx)
{
  auto ty_base = ctx->def_get_type(ref);

  if ((ty_base->get_kind() == AstTypeKind::Int
        && ref.is_global()
        && !static_cast<const AstIntType *>(ty_base)->is_const())
      || ty_base->get_kind() == AstTypeKind::Array
      || ty_base->get_kind() == AstTypeKind::Ptr) {
    auto addr = into_addr(ctx);
    switch (ty_base->get_kind())
    {
    case AstTypeKind::Array:
      if (static_cast<const AstArrayType *>(ty_base)->get_shape().size()
          != indices.size())
        return addr;
      break;
    case AstTypeKind::Ptr:
      if (static_cast<const AstArrayType *>(ty_base)->get_shape().size() + 1
          != indices.size())
        return addr;
      break;
    default:
      break;
    }
    return std::make_unique<HirUnaryExpr>(
        HirUnaryOp::Load, std::move(addr));
  } else if (ty_base->get_kind() == AstTypeKind::Int) {
    if (static_cast<const AstIntType *>(ty_base)->is_const()) {
      int val = ctx->def_get_value(ref);
      return std::make_unique<HirLiteralExpr>(val);
    }
    auto hirid = ctx->def_get_localid(ref);
    assert(indices.size() == 0);
    return std::make_unique<HirLocalVarExpr>(hirid);
  } else {
    abort();
  }
}

std::unique_ptr<HirStmt>
AstLvalExpr::assigned_by(AstContext *ctx, std::unique_ptr<HirExpr> &&rhs)
{
  auto ty_base = ctx->def_get_type(ref);

  if ((ty_base->get_kind() == AstTypeKind::Int && ref.is_global())
      || ty_base->get_kind() == AstTypeKind::Array
      || ty_base->get_kind() == AstTypeKind::Ptr) {
    auto addr = into_addr(ctx);
    auto stmt =
      std::make_unique<HirStoreStmt>(std::move(addr), std::move(rhs));
    return stmt;
  } else if (ty_base->get_kind() == AstTypeKind::Int) {
    auto hirid = ctx->def_get_localid(ref);
    assert(indices.size() == 0);
    return std::make_unique<HirAssignStmt>(hirid, std::move(rhs));
  } else {
    abort();
  }
}

std::unique_ptr<HirExpr> AstLiteralExpr::translate(AstContext *ctx)
{
  return std::make_unique<HirLiteralExpr>(literal);
}

std::unique_ptr<HirExpr> AstCallExpr::translate(AstContext *ctx)
{
  std::vector<std::unique_ptr<HirExpr>> hir_args;
  for (auto &arg : args)
  {
    auto hir_arg = arg->translate(ctx);
    hir_args.emplace_back(std::move(hir_arg));
  }

  return std::make_unique<HirCallExpr>(sym, std::move(hir_args));
}

std::unique_ptr<HirCond>
AstExprCond::translate_into_cond(AstContext *ctx)
{
  auto hir_zero = std::make_unique<HirLiteralExpr>(0);

  if (auto expr_in_not = expr->take_if_unary_not()) {
    auto hir_expr = expr_in_not->translate(ctx);
    return std::make_unique<HirBinaryCond>(
        HirLogicalOp::Eq, std::move(hir_expr), std::move(hir_zero));
  }
  auto hir_expr = expr->translate(ctx);
  return std::make_unique<HirBinaryCond>(
      HirLogicalOp::Ne, std::move(hir_expr), std::move(hir_zero));
}

std::unique_ptr<HirExpr>
AstExprCond::translate_into_expr(AstContext *ctx)
{
  auto hir_expr = expr->translate(ctx);
  return hir_expr;
}

std::unique_ptr<HirCond>
AstBinaryCond::translate_into_cond(AstContext *ctx)
{
  HirLogicalOp hir_lg_op;
  HirShortcutOp hir_sc_op;
  switch (op)
  {
  case AstLogicalOp::And:
    hir_sc_op = HirShortcutOp::And;
    break;
  case AstLogicalOp::Or:
    hir_sc_op = HirShortcutOp::Or;
    break;
  case AstLogicalOp::Lt:
    hir_lg_op = HirLogicalOp::Lt;
    break;
  case AstLogicalOp::Gt:
    hir_lg_op = HirLogicalOp::Gt;
    break;
  case AstLogicalOp::Leq:
    hir_lg_op = HirLogicalOp::Leq;
    break;
  case AstLogicalOp::Geq:
    hir_lg_op = HirLogicalOp::Geq;
    break;
  case AstLogicalOp::Ne:
    hir_lg_op = HirLogicalOp::Ne;
    break;
  case AstLogicalOp::Eq:
    hir_lg_op = HirLogicalOp::Eq;
    break;
  }

  switch (op)
  {
  case AstLogicalOp::And:
  case AstLogicalOp::Or:
    return std::make_unique<HirShortcutCond>(
        hir_sc_op,
        lhs->translate_into_cond(ctx),
        rhs->translate_into_cond(ctx));
  case AstLogicalOp::Lt:
  case AstLogicalOp::Gt:
  case AstLogicalOp::Leq:
  case AstLogicalOp::Geq:
  case AstLogicalOp::Ne:
  case AstLogicalOp::Eq:
    return std::make_unique<HirBinaryCond>(
        hir_lg_op,
        lhs->translate_into_expr(ctx),
        rhs->translate_into_expr(ctx));
  }

  __builtin_unreachable();
}

std::unique_ptr<HirExpr>
AstBinaryCond::translate_into_expr(AstContext *ctx)
{
  HirBinaryOp hir_op;
  switch (op)
  {
  case AstLogicalOp::Lt:
    hir_op = HirBinaryOp::Lt;
    break;
  case AstLogicalOp::Gt:
    hir_op = HirBinaryOp::Gt;
    break;
  case AstLogicalOp::Leq:
    hir_op = HirBinaryOp::Leq;
    break;
  case AstLogicalOp::Geq:
    hir_op = HirBinaryOp::Geq;
    break;
  case AstLogicalOp::Eq:
    hir_op = HirBinaryOp::Eq;
    break;
  case AstLogicalOp::Ne:
    hir_op = HirBinaryOp::Ne;
    break;
  case AstLogicalOp::Or:
  case AstLogicalOp::And:
    abort();
  }

  auto hir_lhs = lhs->translate_into_expr(ctx);
  auto hir_rhs = rhs->translate_into_expr(ctx);

  return std::make_unique<HirBinaryExpr>(hir_op,
      std::move(hir_lhs), std::move(hir_rhs));
}

void AstExprStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  auto hir_expr = expr->translate(ctx);
  auto hir_stmt = std::make_unique<HirExprStmt>(std::move(hir_expr));
  builder->add_statement(std::move(hir_stmt));
}

void AstReturnStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  std::unique_ptr<HirExpr> hir_expr = nullptr;
  if (expr)
    hir_expr = expr->translate(ctx);
  builder->add_statement(
      std::make_unique<HirReturnStmt>(std::move(hir_expr)));
}

static void fill_and_set(
    AstContext *ctx,
    HirFuncBuilder *builder,
    HirArrayId arrayid,
    unsigned int size,
    AstInit::ExprVector &&data)
{
  HirLocalId now = builder->new_local();
  HirLocalId end = builder->new_local();

  builder->add_statement(
      std::make_unique<HirAssignStmt>(now,
        std::make_unique<HirLocalAddrExpr>(arrayid, 0)));
  builder->add_statement(
      std::make_unique<HirAssignStmt>(end,
        std::make_unique<HirBinaryExpr>(HirBinaryOp::Add,
          std::make_unique<HirLocalVarExpr>(now),
          std::make_unique<HirLiteralExpr>((size & ~7u) * 4))));

  auto cond = std::make_unique<HirBinaryCond>(
      HirLogicalOp::Lt,
      std::make_unique<HirLocalVarExpr>(now),
      std::make_unique<HirLocalVarExpr>(end));
  std::vector<std::unique_ptr<HirStmt>> stmts;

  for (unsigned int i = 0; i < 8; ++i)
  {
    auto stmt = std::make_unique<HirStoreStmt>(
        std::make_unique<HirBinaryExpr>(
          HirBinaryOp::Add,
          std::make_unique<HirLocalVarExpr>(now),
          std::make_unique<HirLiteralExpr>(i * 4)),
        std::make_unique<HirLiteralExpr>(0));
    stmts.emplace_back(std::move(stmt));
  }
  stmts.emplace_back(
      std::make_unique<HirAssignStmt>(now,
        std::make_unique<HirBinaryExpr>(
          HirBinaryOp::Add,
          std::make_unique<HirLocalVarExpr>(now),
          std::make_unique<HirLiteralExpr>(8 * 4))));

  builder->add_statement(
      std::make_unique<HirWhileStmt>(std::move(cond),
        std::make_unique<HirBlockStmt>(std::move(stmts))));

  auto it = data.begin();
  while (it != data.end() && it->first < (size & ~7u))
  {
    auto addr = std::make_unique<HirLocalAddrExpr>(arrayid, it->first * 4);
    auto expr = it->second->translate(ctx);
    auto stmt =
      std::make_unique<HirStoreStmt>(std::move(addr), std::move(expr));
    builder->add_statement(std::move(stmt));
    ++it;
  }

  for (unsigned int i = (size & ~7u); i < size; ++i)
  {
    auto addr = std::make_unique<HirLocalAddrExpr>(arrayid, i * 4);
    std::unique_ptr<HirExpr> expr;

    if (it != data.end() && it->first == i) {
      expr = it->second->translate(ctx);
      ++it;
    } else {
      expr = std::make_unique<HirLiteralExpr>(0);
    }

    auto stmt =
      std::make_unique<HirStoreStmt>(std::move(addr), std::move(expr));
    builder->add_statement(std::move(stmt));
  }
}

static void fill_while_setting(
    AstContext *ctx,
    HirFuncBuilder *builder,
    HirArrayId arrayid,
    unsigned int size,
    AstInit::ExprVector &&data)
{
  auto it = data.begin();

  for (unsigned int i = 0; i < size; ++i)
  {
    auto addr = std::make_unique<HirLocalAddrExpr>(arrayid, i * 4);
    std::unique_ptr<HirExpr> expr;

    if (it != data.end() && it->first == i) {
      expr = it->second->translate(ctx);
      ++it;
    } else {
      expr = std::make_unique<HirLiteralExpr>(0);
    }

    auto stmt =
      std::make_unique<HirStoreStmt>(std::move(addr), std::move(expr));
    builder->add_statement(std::move(stmt));
  }
}

void AstDeclStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  for (size_t i = 0; i < sym.size(); ++i)
  {
    auto ty_base = ctx->def_get_type(def[i]);

    if (ty_base->get_kind() == AstTypeKind::Int) {
      auto ty = static_cast<const AstIntType *>(ty_base);
      if (ty->is_const())
        continue;

      HirLocalId localid = builder->new_local();
      ctx->def_set_localid(def[i], localid);
      if (!init[i])
        continue;
  
      auto collected = init[i]->collect(std::vector<unsigned int>());
      assert(collected.size() == 1 && collected[0].first == 0);
      auto hir_expr = collected[0].second->translate(ctx);
      auto hir_stmt =
        std::make_unique<HirAssignStmt>(localid, std::move(hir_expr));
      builder->add_statement(std::move(hir_stmt));
    } else if (ty_base->get_kind() == AstTypeKind::Array) {
      auto ty = static_cast<const AstArrayType *>(ty_base);
  
      if (ty->is_const()) {
        auto collected = init[i]->collect_const(ctx, ty->get_shape());
        auto symbol = def[i].make_unique_symbol();
        ctx->def_set_symbol(def[i], symbol);
        builder->add_item(std::make_unique<HirRodataItem>(
              symbol, ty->num_elems(), std::move(collected)));
      } else {
        HirArrayId arrayid = builder->new_array(ty->num_elems());
        ctx->def_set_arrayid(def[i], arrayid);
        if (!init[i])
          continue;
  
        auto collected = init[i]->collect(ty->get_shape());
        assert(collected.size() <= ty->num_elems());
  
        unsigned int size = ty->num_elems();
        if (collected.size() < size / 2 && size > 16) {
          fill_and_set(ctx, builder,
              arrayid, size, std::move(collected));
        } else {
          fill_while_setting(ctx, builder,
              arrayid, size, std::move(collected));
        }
      }
    } else {
      abort();
    }
  }
}
  
void AstAssignStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  auto hir_rhs = rhs->translate(ctx);
  builder->add_statement(lhs->assigned_by(ctx, std::move(hir_rhs)));
}

std::unique_ptr<HirBlockStmt>
AstBlockStmt::translate_into_block(AstContext *ctx, HirFuncBuilder *builder)
{
  builder->scope_push();

  for (auto &stmt : stmts)
    stmt->translate(ctx, builder);

  return builder->scope_pop();
}

void AstBlockStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  builder->add_statement(translate_into_block(ctx, builder));
}

void AstIfStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  auto hir_cond = cond->translate_into_cond(ctx);
  auto hir_if_stmt = if_stmt->translate_into_block(ctx, builder);

  builder->add_statement(std::make_unique<HirIfStmt>(
        std::move(hir_cond), std::move(hir_if_stmt)));
}

void AstIfElseStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  auto hir_cond = cond->translate_into_cond(ctx);
  auto hir_if_stmt = if_stmt->translate_into_block(ctx, builder);
  auto hir_else_stmt = else_stmt->translate_into_block(ctx, builder);

  builder->add_statement(std::make_unique<HirIfElseStmt>(
        std::move(hir_cond), std::move(hir_if_stmt), std::move(hir_else_stmt)));
}

void AstWhileStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  auto hir_cond = cond->translate_into_cond(ctx);
  auto hir_body = body->translate_into_block(ctx, builder);

  builder->add_statement(std::make_unique<HirWhileStmt>(
        std::move(hir_cond), std::move(hir_body)));
}

void AstBreakStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  builder->add_statement(std::make_unique<HirBreakStmt>());
}

void AstContinueStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  builder->add_statement(std::make_unique<HirContinueStmt>());
}

void AstEmptyStmt::translate(AstContext *ctx, HirFuncBuilder *builder)
{ /* nothing */ }

size_t AstFuncItem::num_items(void) const
{
  return 1;
}

std::unique_ptr<HirItem>
AstFuncItem::translate(AstContext *ctx, size_t i)
{
  std::vector<AstStmt> hir_stmts;
  HirFuncBuilder builder(args.size());

  for (auto &arg : args)
    arg->translate(ctx, &builder);

  for (auto &stmt : body)
    stmt->translate(ctx, &builder);

  return std::make_unique<HirFuncItem>(std::move(builder), sym);
}

size_t AstDeclItem::num_items(void) const
{
  return sym.size();
}

std::unique_ptr<HirItem>
AstDeclItem::translate(AstContext *ctx, size_t i)
{
  auto ty_base = ctx->def_get_type(def[i]);

  if (ty_base->get_kind() == AstTypeKind::Int) {
    auto ty = static_cast<const AstIntType *>(ty_base);
    if (ty->is_const())
      return nullptr;
    ctx->def_set_symbol(def[i], sym[i]);

    if (init[i] != nullptr) {
      auto collected =
        init[i]->collect_const(ctx, std::vector<unsigned int>());
      assert(collected.size() == 1 && collected[0].first == 0);
      return std::make_unique<HirDataItem>(sym[i], 1, std::move(collected));
    }

    return std::make_unique<HirBssItem>(sym[i], 1);
  } else if (ty_base->get_kind() == AstTypeKind::Array) {
    auto ty = static_cast<const AstArrayType *>(ty_base);
    auto sz = ty->num_elems();
    ctx->def_set_symbol(def[i], sym[i]);

    if (init[i] == nullptr) {
      assert(!ty->is_const());
      return std::make_unique<HirBssItem>(sym[i], sz);
    }
    auto collected = init[i]->collect_const(ctx, ty->get_shape());

    if (ty->is_const())
      return std::make_unique<HirRodataItem>(sym[i], sz, std::move(collected));
    else if (collected.size() == 0)
      return std::make_unique<HirBssItem>(sym[i], sz);
    else
      return std::make_unique<HirDataItem>(sym[i], sz, std::move(collected));
  } else {
    abort();
  }
}

void AstFuncArg::translate(AstContext *ctx, HirFuncBuilder *builder)
{
  HirLocalId local = builder->new_local();
  ctx->def_set_localid(def, local);
}

std::unique_ptr<HirCompUnit> AstCompUnit::translate(AstContext *ctx)
{
  std::vector<std::unique_ptr<HirItem>> hir_items;

  for (auto &item : items)
  {
    size_t num = item->num_items();
    for (size_t i = 0; i < num; ++i)
    {
      auto hir_item = item->translate(ctx, i);
      if (hir_item != nullptr)
        hir_items.emplace_back(std::move(hir_item));
    }
  }

  return std::make_unique<HirCompUnit>(std::move(hir_items));
}
