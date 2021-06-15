#include "hir.h"

#define replace_if_constant(variable)           \
  do {                                          \
    if (auto _eval = variable->const_eval())    \
      variable = std::move(_eval);              \
  } while (0)

bool HirExpr::is_literal(void) const
{
  return false;
}

bool HirLiteralExpr::is_literal(void) const
{
  return true;
}

Literal HirExpr::get_literal(void) const
{
  abort();
}

Literal HirLiteralExpr::get_literal(void) const
{
  return literal;
}

bool HirExpr::const_add(Literal value)
{
  return false;
}

bool HirGlobalAddrExpr::const_add(Literal value)
{
  off += value;
  return true;
}

bool HirLocalAddrExpr::const_add(Literal value)
{
  off += value;
  return true;
}

bool HirLiteralExpr::const_add(Literal value)
{
  literal += value;
  return true;
}

bool HirUnaryExpr::const_add(Literal value)
{
  switch (op)
  {
  case HirUnaryOp::Neg:
    return expr->const_add(-value);
  case HirUnaryOp::Not:
  case HirUnaryOp::Load:
    return false;
  }

  __builtin_unreachable();
}

bool HirBinaryExpr::const_add(Literal value)
{
  switch (op)
  {
  case HirBinaryOp::Add:
    return lhs->const_add(value) || rhs->const_add(value);
  case HirBinaryOp::Sub:
    return lhs->const_add(value) || rhs->const_add(-value);
  default:
    return false;
  }

  __builtin_unreachable();
}

bool HirExpr::const_mul(Literal value)
{
  return false;
}

bool HirLiteralExpr::const_mul(Literal value)
{
  literal *= value;
  return true;
}

bool HirUnaryExpr::const_mul(Literal value)
{
  switch (op)
  {
  case HirUnaryOp::Neg:
    return expr->const_mul(value);
  case HirUnaryOp::Not:
  case HirUnaryOp::Load:
    return false;
  }

  __builtin_unreachable();
}

bool HirBinaryExpr::const_mul(Literal value)
{
  switch (op)
  {
  case HirBinaryOp::Mul:
    return lhs->const_mul(value) || rhs->const_mul(value);
  default:
    return false;
  }
}

std::unique_ptr<HirExpr> HirExpr::const_eval(void)
{
  return nullptr;
}

std::unique_ptr<HirExpr> HirUnaryExpr::const_eval(void)
{
  replace_if_constant(expr);

  if (!expr->is_literal())
    return nullptr;

  Literal value = expr->get_literal();
  switch (op)
  {
  case HirUnaryOp::Not:
    value = !value;
    break;
  case HirUnaryOp::Neg:
    value = -value;
    break;
  case HirUnaryOp::Load:
    return nullptr;
  }
  return std::make_unique<HirLiteralExpr>(value);
}

std::unique_ptr<HirExpr> HirBinaryExpr::const_eval(void)
{
  replace_if_constant(lhs);
  replace_if_constant(rhs);

  if (lhs->is_literal() && rhs->is_literal()) {
    Literal lhs_val = lhs->get_literal();
    Literal rhs_val = rhs->get_literal();
    switch (op)
    {
    case HirBinaryOp::Add:
      lhs_val += rhs_val;
      break;
    case HirBinaryOp::Sub:
      lhs_val -= rhs_val;
      break;
    case HirBinaryOp::Mul:
      lhs_val *= rhs_val;
      break;
    case HirBinaryOp::Div:
      if (rhs_val == 0)
        return nullptr;
      lhs_val /= rhs_val;
      break;
    case HirBinaryOp::Mod:
      if (rhs_val == 0)
        return nullptr;
      lhs_val %= rhs_val;
      break;
    case HirBinaryOp::Lt:
      lhs_val = lhs_val < rhs_val;
      break;
    case HirBinaryOp::Gt:
      lhs_val = lhs_val > rhs_val;
      break;
    case HirBinaryOp::Leq:
      lhs_val = lhs_val <= rhs_val;
      break;
    case HirBinaryOp::Geq:
      lhs_val = lhs_val >= rhs_val;
      break;
    case HirBinaryOp::Eq:
      lhs_val = lhs_val == rhs_val;
      break;
    case HirBinaryOp::Ne:
      lhs_val = lhs_val != rhs_val;
      break;
    }
    return std::make_unique<HirLiteralExpr>(lhs_val);
  }

  switch (op)
  {
  case HirBinaryOp::Leq:
    if (!rhs->const_add(1) && !lhs->const_add(-1))
      return std::make_unique<HirUnaryExpr>(HirUnaryOp::Not,
          std::make_unique<HirBinaryExpr>(HirBinaryOp::Lt,
            std::move(rhs), std::move(lhs)));
    op = HirBinaryOp::Lt;
    break;
  case HirBinaryOp::Geq:
    if (!lhs->const_add(1) && !rhs->const_add(-1))
      return std::make_unique<HirUnaryExpr>(HirUnaryOp::Not,
          std::make_unique<HirBinaryExpr>(HirBinaryOp::Lt,
            std::move(lhs), std::move(rhs)));
    std::swap(lhs, rhs);
    op = HirBinaryOp::Lt;
    break;
  case HirBinaryOp::Gt:
    std::swap(lhs, rhs);
    op = HirBinaryOp::Lt;
    break;
  default:
    break;
  }

  if (op == HirBinaryOp::Lt) {
    if (rhs->is_literal()) {
      Literal literal = rhs->get_literal();
      if (literal <= 2047 && literal >= -2048)
        return nullptr;
      if (lhs->const_add(-literal))
        rhs = std::make_unique<HirLiteralExpr>(0);
    } else if (lhs->is_literal()) {
      Literal literal = lhs->get_literal();
      if (rhs->const_add(-literal))
        lhs = std::make_unique<HirLiteralExpr>(0);
    }
    return nullptr;
  }

  if (lhs->is_literal()) {
    switch (op)
    {
    case HirBinaryOp::Add:
    case HirBinaryOp::Mul:
    case HirBinaryOp::Eq:
    case HirBinaryOp::Ne:
      std::swap(lhs, rhs);
      break;
    case HirBinaryOp::Div:
    case HirBinaryOp::Mod:
      break;
    default:
      abort();
    }
  }

  if (rhs->is_literal()) {
    Literal val = rhs->get_literal();
    switch (op)
    {
    case HirBinaryOp::Add:
      if (val == 0 || lhs->const_add(val))
        return std::move(lhs);
      break;
    case HirBinaryOp::Sub:
      if (val == 0 || lhs->const_add(-val))
        return std::move(lhs);
      break;
    case HirBinaryOp::Mul:
      if (val == 0)
        return std::make_unique<HirLiteralExpr>(0);
      if (val == 1 || lhs->const_mul(val))
        return std::move(lhs);
      break;
    case HirBinaryOp::Div:
      if (val == 1)
        return std::move(lhs);
      break;
    case HirBinaryOp::Mod:
      if (val == 1)
        return std::make_unique<HirLiteralExpr>(0);
      break;
    case HirBinaryOp::Eq:
    case HirBinaryOp::Ne:
      if (val == 0)
        return nullptr;
      if (lhs->const_add(-val)) {
        rhs = std::make_unique<HirLiteralExpr>(0);
        return nullptr;
      }
      break;
    default:
      abort();
    }
  }

  if (op == HirBinaryOp::Eq
      || op == HirBinaryOp::Ne) {
    lhs = std::make_unique<HirBinaryExpr>(
        HirBinaryOp::Sub, std::move(lhs), std::move(rhs));
    rhs = std::make_unique<HirLiteralExpr>(0);
  }

  return nullptr;
}

bool HirCond::is_literal(void) const
{
  return false;
}

bool HirCond::get_literal(void) const
{
  abort();
}

bool HirTrueCond::is_literal(void) const
{
  return true;
}

bool HirTrueCond::get_literal(void) const
{
  return true;
}

bool HirFalseCond::is_literal(void) const
{
  return true;
}

bool HirFalseCond::get_literal(void) const
{
  return false;
}

std::unique_ptr<HirCond> HirCond::const_eval(void)
{
  return nullptr;
}

std::unique_ptr<HirCond> HirBinaryCond::const_eval(void)
{
  replace_if_constant(lhs);
  replace_if_constant(rhs);

  if (lhs->is_literal() && rhs->is_literal()) {
    Literal lhs_val = lhs->get_literal();
    Literal rhs_val = rhs->get_literal();

    switch (op)
    {
    case HirLogicalOp::Lt:
      lhs_val = lhs_val < rhs_val;
      break;
    case HirLogicalOp::Gt:
      lhs_val = lhs_val > rhs_val;
      break;
    case HirLogicalOp::Leq:
      lhs_val = lhs_val <= rhs_val;
      break;
    case HirLogicalOp::Geq:
      lhs_val = lhs_val >= rhs_val;
      break;
    case HirLogicalOp::Eq:
      lhs_val = lhs_val == rhs_val;
      break;
    case HirLogicalOp::Ne:
      lhs_val = lhs_val != rhs_val;
      break;
    }

    if (lhs_val)
      return std::make_unique<HirTrueCond>();
    else
      return std::make_unique<HirFalseCond>();
  }

  if (lhs->is_literal()) {
    Literal val = lhs->get_literal();
    if (val && rhs->const_add(-val)) {
      lhs = std::make_unique<HirLiteralExpr>(0);
      return nullptr;
    }
    if (val && val <= 2048 && val >= -2047) {
      rhs = std::make_unique<HirBinaryExpr>(
          HirBinaryOp::Add,
          std::move(rhs),
          std::make_unique<HirLiteralExpr>(-val));
      lhs = std::make_unique<HirLiteralExpr>(0);
      return nullptr;
    }
  } else if (rhs->is_literal()) {
    Literal val = rhs->get_literal();
    if (val && lhs->const_add(-val)) {
      rhs = std::make_unique<HirLiteralExpr>(0);
      return nullptr;
    }
    if (val && val <= 2048 && val >= -2047) {
      lhs = std::make_unique<HirBinaryExpr>(
          HirBinaryOp::Add,
          std::move(lhs),
          std::make_unique<HirLiteralExpr>(-val));
      rhs = std::make_unique<HirLiteralExpr>(0);
      return nullptr;
    }
  }

  return nullptr;
}

std::unique_ptr<HirCond> HirShortcutCond::const_eval(void)
{
  replace_if_constant(lhs);
  replace_if_constant(rhs);

  if (!lhs->is_literal())
    return nullptr;
  bool val = lhs->get_literal();

  switch (op)
  {
  case HirShortcutOp::Or:
    if (val)
      return std::make_unique<HirTrueCond>();
    else
      return std::move(rhs);
  case HirShortcutOp::And:
    if (val)
      return std::move(rhs);
    else
      return std::make_unique<HirFalseCond>();
  }

  __builtin_unreachable();
}

std::unique_ptr<HirExpr> HirCallExpr::const_eval(void)
{
  for (auto &arg : args)
    replace_if_constant(arg);

  return nullptr;
}

void HirStoreStmt::const_eval(void)
{
  replace_if_constant(addr);
  replace_if_constant(val);
}

void HirReturnStmt::const_eval(void)
{
  if (expr != nullptr)
    replace_if_constant(expr);
}

void HirBlockStmt::const_eval(void)
{
  for (auto &stmt : stmts)
    stmt->const_eval();
}

void HirIfStmt::const_eval(void)
{
  replace_if_constant(cond);
  if_stmt->const_eval();
}

void HirIfElseStmt::const_eval(void)
{
  replace_if_constant(cond);
  if_stmt->const_eval();
  else_stmt->const_eval();
}

void HirWhileStmt::const_eval(void)
{
  replace_if_constant(cond);
  body->const_eval();
}

void HirExprStmt::const_eval(void)
{
  replace_if_constant(expr);
}

void HirAssignStmt::const_eval(void)
{
  replace_if_constant(rhs);
}

void HirContinueStmt::const_eval(void)
{ /* nothing */ }

void HirBreakStmt::const_eval(void)
{ /* nothing */ }

void HirItem::const_eval(void)
{ /* nothing */ }

void HirFuncItem::const_eval(void)
{
  body->const_eval();
}

void HirCompUnit::const_eval(void)
{
  for (auto &item : items)
    item->const_eval();
}
