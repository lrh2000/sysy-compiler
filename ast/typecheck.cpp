#include <iostream>
#include "ast.h"
#include "context.h"
#include "type.h"

[[ noreturn ]]
static void expected_but_found(const AstType *expected,
                               const AstType *found)
{
  std::cerr << "error: expected "
            << *expected
            << ", found "
            << *found
            << std::endl;
  abort();
}

static inline void coerce_or_err(const AstType *target,
                                 const AstType *source)
{
  if (source->get_kind() == AstTypeKind::Int)
    source = AstType::mk_int(false);
  if (source != target)
    expected_but_found(target, source);
}

static inline void enforce_int(const AstType *ty)
{
  if (ty->get_kind() != AstTypeKind::Int)
    return expected_but_found(AstType::mk_int(false), ty);
}

const AstType *AstBinaryExpr::type_check(AstContext *ctx)
{
  enforce_int(lhs->type_check(ctx));
  enforce_int(rhs->type_check(ctx));
  return AstType::mk_int(false);
}

const AstType *AstUnaryExpr::type_check(AstContext *ctx)
{
  enforce_int(expr->type_check(ctx));
  return AstType::mk_int(false);
}

const AstType *AstLvalExpr::type_check(AstContext *ctx)
{
  for (auto &index : indices)
    enforce_int(index->type_check(ctx));

  auto ty_base = ctx->def_get_type(ref);

  if (indices.size() > 0
      || ty_base->get_kind() == AstTypeKind::Array
      || ty_base->get_kind() == AstTypeKind::Ptr) {
    if (ty_base->get_kind() == AstTypeKind::Array) {
      auto ty = static_cast<const AstArrayType *>(ty_base);
      if (ty->get_shape().size() < indices.size()) {
        std::cerr << "error: too many subscripts on type "
                  << *ty
                  << std::endl;
        abort();
      } else if (ty->get_shape().size() == indices.size()) {
        return AstType::mk_int(ty->is_const());
      } else {
        std::vector<unsigned int> shape;
        for (size_t i = indices.size() + 1;
             i < ty->get_shape().size();
             ++i)
          shape.emplace_back(ty->get_shape()[i]);
        return AstType::mk_ptr(ty->is_const(), std::move(shape));
      }
    } else if (ty_base->get_kind() == AstTypeKind::Ptr) {
      auto ty = static_cast<const AstPtrType *>(ty_base);
      if (ty->get_shape().size() + 1 < indices.size()) {
        std::cerr << "error: too many subscripts on type "
                  << *ty
                  << std::endl;
        abort();
      } else if (ty->get_shape().size() + 1 == indices.size()) {
        return AstType::mk_int(ty->is_const());
      } else {
        std::vector<unsigned int> shape;
        for (size_t i = indices.size();
             i < ty->get_shape().size();
             ++i)
          shape.emplace_back(ty->get_shape()[i]);
        return AstType::mk_ptr(ty->is_const(), std::move(shape));
      }
    } else {
      std::cerr << "error: expected array or pointer, found "
                << *ty_base
                << std::endl;
      abort();
    }
  } else {
    enforce_int(ty_base);
    return ty_base;
  }
}

const AstType *AstLiteralExpr::type_check(AstContext *ctx)
{
  return AstType::mk_int(false);
}

const AstType *AstCallExpr::type_check(AstContext *ctx)
{
  auto ty_base = ctx->def_get_type(ref);
  if (ty_base->get_kind() != AstTypeKind::Func) {
    std::cerr << "error: object with type "
              << *ty_base
              << " is not callable"
              << std::endl;
    abort();
  }

  auto ty = static_cast<const AstFuncType *>(ty_base);
  if (ty->get_arg_ty().size() > args.size()) {
    std::cerr << "error: too few arguments to invoke "
              << *ty
              << std::endl;
    abort();
  } else if (ty->get_arg_ty().size() < args.size()) {
    std::cerr << "error: too many arguments to invoke "
              << *ty
              << std::endl;
    abort();
  }

  for (size_t i = 0; i < args.size(); ++i)
  {
    auto target = ty->get_arg_ty()[i];
    auto source = args[i]->type_check(ctx);
    coerce_or_err(target, source);
  }

  return ty->get_ret_ty();
}

void AstExprCond::type_check(AstContext *ctx)
{
  enforce_int(expr->type_check(ctx));
}

void AstBinaryCond::type_check(AstContext *ctx)
{
  lhs->type_check(ctx);
  rhs->type_check(ctx);
}

void AstExprInit::type_check(AstContext *ctx)
{
  enforce_int(expr->type_check(ctx));
}

void AstListInit::type_check(AstContext *ctx)
{
  for (auto &elem : list)
    elem->type_check(ctx);
}

void AstExprStmt::type_check(AstContext *ctx)
{
  expr->type_check(ctx);
}

void AstReturnStmt::type_check(AstContext *ctx)
{
  auto func_ty = ctx->func_top();

  auto target = func_ty->get_ret_ty();
  const AstType *source;
  if (expr == nullptr)
    source = AstType::mk_void();
  else
    source = expr->type_check(ctx);
  coerce_or_err(target, source);
}

void AstDeclStmt::type_check(AstContext *ctx)
{
  for (size_t i = 0; i < sym.size(); ++i)
  {
    std::vector<unsigned int> shape;

    for (auto &index : indices[i])
    {
      enforce_int(index->type_check(ctx));
      int s = index->const_eval(ctx);
      if (s <= 0) {
        std::cerr << "error: negative size for array "
                  << sym[i]
                  << " is not allowed"
                  << std::endl;
        abort();
      }
      shape.emplace_back(static_cast<unsigned int>(s));
    }

    const AstType *ty;
    if (shape.size() == 0)
      ty = AstType::mk_int(is_const);
    else
      ty = AstType::mk_array(is_const, std::move(shape));
    ctx->def_set_type(def[i], ty);

    if (indices[i].size() == 0 && is_const) {
      assert(init[i] != nullptr);
      init[i]->type_check(ctx);
      auto values = init[i]->collect_const(
          ctx, std::vector<unsigned int>());
      assert(values.size() == 1 && values[0].first == 0);
      ctx->def_set_value(def[i], values[0].second);
      continue;
    }

    if (init[i] != nullptr)
      init[i]->type_check(ctx);
  }
}

void AstAssignStmt::type_check(AstContext *ctx)
{
  auto ty = lhs->type_check(ctx);
  if (ty != AstType::mk_int(false)) {
    std::cerr << "error: type "
              << *ty
              << " is not assignable"
              << std::endl;
    abort();
  }

  enforce_int(rhs->type_check(ctx));
}

void AstBlockStmt::type_check(AstContext *ctx)
{
  for (auto &stmt: stmts)
    stmt->type_check(ctx);
}

void AstIfStmt::type_check(AstContext *ctx)
{
  cond->type_check(ctx);
  if_stmt->type_check(ctx);
}

void AstIfElseStmt::type_check(AstContext *ctx)
{
  cond->type_check(ctx);
  if_stmt->type_check(ctx);
  else_stmt->type_check(ctx);
}

void AstWhileStmt::type_check(AstContext *ctx)
{
  cond->type_check(ctx);
  body->type_check(ctx);
}

void AstBreakStmt::type_check(AstContext *ctx)
{ /* nothing */ }

void AstContinueStmt::type_check(AstContext *ctx)
{ /* nothing */ }

void AstEmptyStmt::type_check(AstContext *ctx)
{ /* nothing */ }

const AstType *AstFuncArg::type_check(AstContext *ctx)
{
  if (indices.size() == 0) {
    auto ty = AstType::mk_int(false);
    ctx->def_set_type(def, ty);
    return ty;
  }

  std::vector<unsigned int> shape;
  for (size_t i = 1; i < indices.size(); ++i)
  {
    enforce_int(indices[i]->type_check(ctx));
    int s = indices[i]->const_eval(ctx);
    if (s <= 0) {
      std::cerr << "error: negative size for array "
                << sym
                << " is not allowed"
                << std::endl;
      abort();
    }
    shape.emplace_back(static_cast<unsigned int>(s));
  }

  auto ty = AstType::mk_ptr(false, std::move(shape));
  ctx->def_set_type(def, ty);
  return ty;
}

void AstFuncItem::type_check(AstContext *ctx)
{
  const AstType *ret_ty;
  if (is_void)
    ret_ty = AstType::mk_void();
  else
    ret_ty = AstType::mk_int(false);

  std::vector<const AstType *> arg_ty;
  for (auto &arg : args)
    arg_ty.emplace_back(arg->type_check(ctx));

  auto func_ty = AstType::mk_func(ret_ty, std::move(arg_ty));
  ctx->def_set_type(def, func_ty);

  ctx->func_push(static_cast<const AstFuncType *>(func_ty));

  for (auto &stmt : body)
    stmt->type_check(ctx);

  ctx->func_pop();
}

void AstDeclItem::type_check(AstContext *ctx)
{
  for (size_t i = 0; i < sym.size(); ++i)
  {
    std::vector<unsigned int> shape;

    for (auto &index : indices[i])
    {
      enforce_int(index->type_check(ctx));
      int s = index->const_eval(ctx);
      if (s <= 0) {
        std::cerr << "error: negative size for array "
                  << sym[i]
                  << " is not allowed"
                  << std::endl;
        abort();
      }
      shape.emplace_back(static_cast<unsigned int>(s));
    }

    const AstType *ty;
    if (shape.size() == 0)
      ty = AstType::mk_int(is_const);
    else
      ty = AstType::mk_array(is_const, std::move(shape));
    ctx->def_set_type(def[i], ty);

    if (indices[i].size() == 0 && is_const) {
      assert(init[i] != nullptr);
      init[i]->type_check(ctx);
      auto values = init[i]->collect_const(
          ctx, std::vector<unsigned int>());
      assert(values.size() == 1 && values[0].first == 0);
      ctx->def_set_value(def[i], values[0].second);
      continue;
    }

    if (init[i] != nullptr)
      init[i]->type_check(ctx);
  }
}

void AstCompUnit::type_check(AstContext *ctx)
{
  const AstType *ty_int = AstType::mk_int(false);
  const AstType *ty_void = AstType::mk_void();
  const AstType *ty_ptr =
    AstType::mk_ptr(false, std::vector<unsigned int>());

  ctx->def_set_type(
      prelude_defs[0],
      AstType::mk_func(ty_int,
        std::vector<const AstType *>()));

  ctx->def_set_type(
      prelude_defs[1],
      AstType::mk_func(ty_void,
        std::vector<const AstType *> { ty_int }));

  ctx->def_set_type(
      prelude_defs[2],
      AstType::mk_func(ty_int,
        std::vector<const AstType *>()));

  ctx->def_set_type(
      prelude_defs[3],
      AstType::mk_func(ty_void,
        std::vector<const AstType *> { ty_int }));

  ctx->def_set_type(
      prelude_defs[4],
      AstType::mk_func(ty_int,
        std::vector<const AstType *> { ty_ptr }));

  ctx->def_set_type(
      prelude_defs[5],
      AstType::mk_func(ty_void,
        std::vector<const AstType *> { ty_int, ty_ptr }));

  ctx->def_set_type(
      prelude_defs[6],
      AstType::mk_func(ty_void,
        std::vector<const AstType *> { ty_int }));

  ctx->def_set_type(
      prelude_defs[7],
      AstType::mk_func(ty_void,
        std::vector<const AstType *> { ty_int }));

  for (auto &item : items)
    item->type_check(ctx);
}
