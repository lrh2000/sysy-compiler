#include "hir.h"
#include "../mir/mir.h"
#include "../mir/builder.h"

MirLocal HirExpr::translate(MirFuncBuilder *builder)
{
  MirLocal temp = builder->new_temp();
  translate(builder, temp);
  return temp;
}

void HirGlobalAddrExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  builder->add_statement(
      std::make_unique<MirSymbolAddrStmt>(dest, name, off));
}

void HirLocalAddrExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  builder->add_statement(
      std::make_unique<MirArrayAddrStmt>(dest, (MirArray) vid, off));
}

MirLocal HirLocalVarExpr::translate(MirFuncBuilder *builder)
{
  return (MirLocal) vid;
}

void HirLocalVarExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  builder->add_statement(
      std::make_unique<MirUnaryStmt>(dest, (MirLocal) vid, MirUnaryOp::Nop));
}

MirLocal HirLiteralExpr::translate(MirFuncBuilder *builder)
{
  if (literal == 0)
    return ~0u;
  MirLocal temp = builder->new_temp();
  translate(builder, temp);
  return temp;
}

void HirLiteralExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  builder->add_statement(
      std::make_unique<MirImmStmt>(dest, literal));
}

void HirUnaryExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  MirUnaryOp mir_op;
  MirLocal temp;

  temp = expr->translate(builder);
  switch (op)
  {
  case HirUnaryOp::Neg:
    mir_op = MirUnaryOp::Neg;
    break;
  case HirUnaryOp::Not:
    mir_op = MirUnaryOp::Eqz;
    break;
  case HirUnaryOp::Load:
    builder->add_statement(std::make_unique<MirLoadStmt>(dest, temp, 0));
    return;
  }

  builder->add_statement(std::make_unique<MirUnaryStmt>(dest, temp, mir_op));
}

void HirBinaryExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  MirLocal mir_lhs = lhs->translate(builder);

  if (rhs->is_literal()) {
    Literal val = rhs->get_literal();
    switch (op)
    {
    case HirBinaryOp::Add:
      if (val <= 2047 && val >= -2048) {
        builder->add_statement(
            std::make_unique<MirBinaryImmStmt>(
              dest, mir_lhs, val, MirImmOp::Add));
        return;
      }
      break;
    case HirBinaryOp::Sub:
      if (val <= 2048 && val >= -2047) {
        builder->add_statement(
            std::make_unique<MirBinaryImmStmt>(
              dest, mir_lhs, -val, MirImmOp::Add));
        return;
      }
      break;
    case HirBinaryOp::Mul:
      if (val > 0 && (val & (val - 1)) == 0) {
        builder->add_statement(
            std::make_unique<MirBinaryImmStmt>(
              dest, mir_lhs, val, MirImmOp::Mul));
        return;
      }
      break;
    case HirBinaryOp::Lt:
      if (val <= 2047 && val >= -2048) {
        builder->add_statement(
            std::make_unique<MirBinaryImmStmt>(
              dest, mir_lhs, val, MirImmOp::Lt));
        return;
      }
      break;
    default:
      break;
    }
  }

  MirLocal mir_rhs = rhs->translate(builder);
  MirBinaryOp mir_op;

  switch (op)
  {
  case HirBinaryOp::Add:
    mir_op = MirBinaryOp::Add;
    break;
  case HirBinaryOp::Sub:
    mir_op = MirBinaryOp::Sub;
    break;
  case HirBinaryOp::Mul:
    mir_op = MirBinaryOp::Mul;
    break;
  case HirBinaryOp::Div:
    mir_op = MirBinaryOp::Div;
    break;
  case HirBinaryOp::Mod:
    mir_op = MirBinaryOp::Mod;
    break;
  case HirBinaryOp::Lt:
    mir_op = MirBinaryOp::Lt;
    break;
  case HirBinaryOp::Eq:
    assert(rhs->is_literal() && rhs->get_literal() == 0);
    builder->add_statement(
        std::make_unique<MirUnaryStmt>(dest, mir_lhs, MirUnaryOp::Eqz));
    break;
  case HirBinaryOp::Ne:
    assert(rhs->is_literal() && rhs->get_literal() == 0);
    builder->add_statement(
        std::make_unique<MirUnaryStmt>(dest, mir_lhs, MirUnaryOp::Nez));
    break;
  case HirBinaryOp::Leq:
  case HirBinaryOp::Gt:
  case HirBinaryOp::Geq:
    abort();
  }

  builder->add_statement(
      std::make_unique<MirBinaryStmt>(dest, mir_lhs, mir_rhs, mir_op));
}

void HirCallExpr::translate(MirFuncBuilder *builder, MirLocal dest)
{
  std::vector<MirLocal> locals;

  for (auto &arg : args)
  {
    MirLocal temp = arg->translate(builder);
    locals.emplace_back(temp);
  }

  builder->add_statement(
      std::make_unique<MirCallStmt>(dest, name, std::move(locals)));
}

void HirTrueCond::translate(
    MirFuncBuilder *builder, MirLabel label_t, MirLabel label_f)
{ /* nothing */ }

void HirFalseCond::translate(
    MirFuncBuilder *builder, MirLabel label_t, MirLabel label_f)
{
  builder->add_statement(std::make_unique<MirJumpStmt>(label_f));
}

void HirBinaryCond::translate(
    MirFuncBuilder *builder, MirLabel label_t, MirLabel label_f)
{
  MirLogicalOp mir_op;
  bool swap;
  switch (op)
  {
  case HirLogicalOp::Lt:
    mir_op = MirLogicalOp::Leq;
    swap = true;
    break;
  case HirLogicalOp::Leq:
    mir_op = MirLogicalOp::Lt;
    swap = true;
    break;
  case HirLogicalOp::Gt:
    mir_op = MirLogicalOp::Leq;
    swap = false;
    break;
  case HirLogicalOp::Geq:
    mir_op = MirLogicalOp::Lt;
    swap = false;
    break;
  case HirLogicalOp::Eq:
    mir_op = MirLogicalOp::Ne;
    swap = false;
    break;
  case HirLogicalOp::Ne:
    mir_op = MirLogicalOp::Eq;
    swap = false;
    break;
  }

  MirLocal mir_lhs = lhs->translate(builder);
  MirLocal mir_rhs = rhs->translate(builder);
  if (swap)
    std::swap(mir_lhs, mir_rhs);

  builder->add_statement(
      std::make_unique<MirBranchStmt>(
        mir_lhs, mir_rhs, label_f, mir_op));
}

void HirShortcutCond::translate(
    MirFuncBuilder *builder, MirLabel label_t, MirLabel label_f)
{
  MirLabel middle = builder->new_label();

  switch (op)
  {
  case HirShortcutOp::And:
    lhs->translate(builder, middle, label_f);
    builder->set_label(middle);
    rhs->translate(builder, label_t, label_f);
    break;
  case HirShortcutOp::Or:
    lhs->translate(builder, label_t, middle);
    builder->add_statement(std::make_unique<MirJumpStmt>(label_t));
    builder->set_label(middle);
    rhs->translate(builder, label_t, label_f);
    break;
  }
}

void HirStoreStmt::translate(MirFuncBuilder *builder)
{
  MirLocal addr = this->addr->translate(builder);
  MirLocal val = this->val->translate(builder);
  builder->add_statement(std::make_unique<MirStoreStmt>(val, addr, 0));
}

void HirReturnStmt::translate(MirFuncBuilder *builder)
{
  if (!expr) {
    builder->add_statement(std::make_unique<MirReturnStmt>());
  } else {
    MirLocal temp = expr->translate(builder);
    builder->add_statement(std::make_unique<MirReturnStmt>(temp));
  }
}

void HirBlockStmt::translate(MirFuncBuilder *builder)
{
  for (auto &stmt : stmts)
    stmt->translate(builder);
}

void HirIfStmt::translate(MirFuncBuilder *builder)
{
  MirLabel if_label = builder->new_label();
  MirLabel else_label = builder->new_label();
  MirLabel end_label = builder->new_label();

  cond->translate(builder, if_label, else_label);

  builder->set_label(if_label);
  if_stmt->translate(builder);
  builder->add_statement(std::make_unique<MirJumpStmt>(end_label));

  builder->set_label(else_label);
  /* nothing */

  builder->set_label(end_label);
}

void HirIfElseStmt::translate(MirFuncBuilder *builder)
{
  MirLabel if_label = builder->new_label();
  MirLabel else_label = builder->new_label();
  MirLabel end_label = builder->new_label();

  cond->translate(builder, if_label, else_label);

  builder->set_label(if_label);
  if_stmt->translate(builder);
  builder->add_statement(std::make_unique<MirJumpStmt>(end_label));

  builder->set_label(else_label);
  else_stmt->translate(builder);

  builder->set_label(end_label);
}

void HirWhileStmt::translate(MirFuncBuilder *builder)
{
  MirLabel head = builder->new_label();
  MirLabel body = builder->new_label();
  MirLabel branch_tail = builder->new_label();
  MirLabel jump_tail = builder->new_label();

  builder->loop_push(head, jump_tail);

  builder->set_label(head);
  cond->translate(builder, body, branch_tail);

  builder->set_label(body);
  this->body->translate(builder);

  builder->add_statement(std::make_unique<MirJumpStmt>(head));
  builder->set_label(branch_tail);
  builder->set_label(jump_tail);

  builder->loop_pop();
}

void HirExprStmt::translate(MirFuncBuilder *builder)
{
  expr->translate(builder);
}

void HirAssignStmt::translate(MirFuncBuilder *builder)
{
  MirLocal temp = builder->new_temp();
  rhs->translate(builder, temp);
  builder->add_statement(
      std::make_unique<MirUnaryStmt>((MirLocal) lhs, temp, MirUnaryOp::Nop));
}

void HirContinueStmt::translate(MirFuncBuilder *builder)
{
  MirLabel label = builder->loop_get_head();
  builder->add_statement(std::make_unique<MirJumpStmt>(label));
}

void HirBreakStmt::translate(MirFuncBuilder *builder)
{
  MirLabel label = builder->loop_get_tail();
  builder->add_statement(std::make_unique<MirJumpStmt>(label));
}

void HirFuncItem::translate(MirBuilder *builder)
{
  for (auto &item : items)
    item->translate(builder);

  MirFuncBuilder func_builder(name, num_args,
      num_locals, array_sz, std::move(array_off));

  body->translate(&func_builder);
  func_builder.finish_build();

  builder->add_item(
      std::make_unique<MirFuncItem>(std::move(func_builder)));
}

void HirDataItem::translate(MirBuilder *builder)
{
  builder->add_item(
      std::make_unique<MirDataItem>(name, size, std::move(values)));
}

void HirRodataItem::translate(MirBuilder *builder)
{
  builder->add_item(
      std::make_unique<MirRodataItem>(name, size, std::move(values)));
}

void HirBssItem::translate(MirBuilder *builder)
{
  builder->add_item(std::make_unique<MirBssItem>(name, size));
}

std::unique_ptr<MirCompUnit> HirCompUnit::translate(void)
{
  MirBuilder builder;

  for (auto &item : items)
    item->translate(&builder);

  return std::make_unique<MirCompUnit>(std::move(builder));
}
