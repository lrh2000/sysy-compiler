#include <iostream>
#include "ast.h"
#include "context.h"
#include "type.h"

int AstBinaryExpr::const_eval(const AstContext *ctx) const
{
  int left = lhs->const_eval(ctx);
  int right = rhs->const_eval(ctx);

  switch (op)
  {
  case AstBinaryOp::Add:
    left += right;
    break;
  case AstBinaryOp::Sub:
    left -= right;
    break;
  case AstBinaryOp::Mul:
    left *= right;
    break;
  case AstBinaryOp::Div:
    if (right == 0) {
      std::cerr << "error: dividing by zero is not allowed in constant expression"
                << std::endl;
      abort();
    }
    left /= right;
    break;
  case AstBinaryOp::Mod:
    if (right == 0) {
      std::cerr << "error: dividing by zero is not allowed in constant expression"
                << std::endl;
      abort();
    }
    left %= right;
    break;
  }

  return left;
}

int AstUnaryExpr::const_eval(const AstContext *ctx) const
{
  int val = expr->const_eval(ctx);

  switch (op)
  {
  case AstUnaryOp::Not:
    val = !val;
    break;
  case AstUnaryOp::Pos:
    val = +val;
    break;
  case AstUnaryOp::Neg:
    val = -val;
    break;
  }

  return val;
}

int AstLvalExpr::const_eval(const AstContext *ctx) const
{
  if (indices.size() > 0) {
    std::cerr << "error: accessing array element is not allowed in constant expression"
              << std::endl;
    abort();
  }

  auto expected = AstType::mk_int(true);
  auto found = ctx->def_get_type(ref);
  if (expected != found) {
    std::cerr << "error: expected "
              << *expected
              << ", found "
              << *found
              << std::endl;
    abort();
  }

  return ctx->def_get_value(ref);
}

int AstLiteralExpr::const_eval(const AstContext *ctx) const
{
  return literal;
}

int AstCallExpr::const_eval(const AstContext *ctx) const
{
  std::cerr << "error: calling function is not allowed in constant expression"
            << std::endl;
  abort();
}

AstInit::ExprVector
AstInit::collect(const std::vector<unsigned int> &shape)
{
  AstInit::ExprVector result;
  do_collect_all(shape, 0, result, 0);
  return result;
}

AstInit::LiteralVector
AstInit::collect_const(const AstContext *ctx,
                       const std::vector<unsigned int> &shape)
{
  auto expr = collect(shape);
  AstInit::LiteralVector lit;

  for (auto &e : expr)
  {
    int val = e.second->const_eval(ctx);
    if (val != 0 || shape.size() == 0)
      lit.emplace_back(e.first, val);
  }

  return lit;
}

void AstExprInit::do_collect_all(
    const std::vector<unsigned int> &shape,
    size_t depth,
    std::vector<std::pair<unsigned int, std::unique_ptr<AstExpr>>> &result,
    size_t base)
{
  if (depth != shape.size()) {
    std::cerr << "error: scalar initializer is invalid for array"
              << std::endl;
    abort();
  }

  (void) do_collect(0, shape, depth, result, base);
}

size_t AstExprInit::do_collect(
    size_t position,
    const std::vector<unsigned int> &shape,
    size_t depth,
    std::vector<std::pair<unsigned int, std::unique_ptr<AstExpr>>> &result,
    size_t base)
{
  assert(position == 0);
  assert(depth == shape.size());
  assert(expr != nullptr);

  result.emplace_back(base, std::move(expr));
  return position + 1;
}

void AstListInit::do_collect_all(
    const std::vector<unsigned int> &shape,
    size_t depth,
    std::vector<std::pair<unsigned int, std::unique_ptr<AstExpr>>> &result,
    size_t base)
{
  if (shape.size() == depth) {
    std::cerr << "error: excess braces around scalar initializer"
              << std::endl;
    abort();
  }

  size_t position = do_collect(0, shape, depth, result, base);
  if (position < list.size()) {
    std::cerr << "error: excess elements in array initializer" 
              << std::endl;
    abort();
  }
}

size_t AstListInit::do_collect(
    size_t position,
    const std::vector<unsigned int> &shape,
    size_t depth,
    std::vector<std::pair<unsigned int, std::unique_ptr<AstExpr>>> &result,
    size_t base)
{
  assert(depth <= shape.size() && position <= list.size());
  if (depth == shape.size()) {
    if (position < list.size()) {
      list[position]->do_collect_all(shape, depth, result, base);
      return position + 1;
    } else {
      return position;
    }
  }
  if (position == list.size())
    return position;

  unsigned int step = 1;
  for (size_t i = shape.size() - 1; i > depth; --i)
    step *= shape[i];

  for (unsigned int i = 0; i < shape[depth]; ++i, base += step)
  {
    if (position == list.size())
      return position;
    assert(position < list.size());

    if (list[position]->is_list()) {
      auto child_list = static_cast<AstListInit *>(list[position].get());
      child_list->do_collect_all(shape, depth + 1, result, base);
      position += 1;
    } else {
      position = do_collect(position, shape, depth + 1, result, base);
    }
  }
  return position;
}
