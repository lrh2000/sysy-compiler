#pragma once
#include <memory>
#include <vector>
#include "../lexer/symbol.h"
#include "defid.h"
#include "../mir/defid.h"

enum class HirBinaryOp
{
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Lt,
  Gt,
  Leq,
  Geq,
  Eq,
  Ne,
};

enum class HirUnaryOp
{
  Not,
  Neg,
  Load,
};

enum class HirLogicalOp
{
  Lt,
  Gt,
  Leq,
  Geq,
  Eq,
  Ne,
};

enum class HirShortcutOp
{
  And,
  Or,
};

class MirFuncBuilder;
class MirBuilder;

class MirCompUnit;

class HirFuncBuilder;

class HirExpr
{
public:
  virtual void translate(MirFuncBuilder *builder, MirLocal dest) = 0;
  virtual MirLocal translate(MirFuncBuilder *builder);

  virtual std::unique_ptr<HirExpr> const_eval(void);

  virtual bool is_literal(void) const;
  virtual Literal get_literal(void) const;

  virtual bool const_add(int value);
  virtual bool const_mul(int value);
};

class HirGlobalAddrExpr :public HirExpr
{
public:
  HirGlobalAddrExpr(Symbol name, off_t off)
    : name(name), off(off)
  {}

  void translate(MirFuncBuilder *builder, MirLocal dest) override;

  bool const_add(int value) override;

private:
  Symbol name;
  off_t off;
};

class HirLocalAddrExpr :public HirExpr
{
public:
  HirLocalAddrExpr(HirArrayId vid, off_t off)
    : vid(vid), off(off)
  {}

  void translate(MirFuncBuilder *builder, MirLocal dest) override;

  bool const_add(int value) override;

private:
  HirArrayId vid;
  off_t off;
};

class HirLocalVarExpr :public HirExpr
{
public:
  HirLocalVarExpr(HirLocalId vid)
    : vid(vid)
  {}

  MirLocal translate(MirFuncBuilder *builder) override;
  void translate(MirFuncBuilder *builder, MirLocal dest) override;

private:
  HirLocalId vid;
};

class HirLiteralExpr :public HirExpr
{
public:
  HirLiteralExpr(Literal literal)
    : literal(literal)
  {}

  MirLocal translate(MirFuncBuilder *builder) override;
  void translate(MirFuncBuilder *builder, MirLocal dest) override;

  bool is_literal(void) const override;
  Literal get_literal(void) const override;

  bool const_add(int value) override;
  bool const_mul(int value) override;

private:
  Literal literal;
};

class HirUnaryExpr :public HirExpr
{
public:
  HirUnaryExpr(HirUnaryOp op, std::unique_ptr<HirExpr> &&expr)
    : op(op), expr(std::move(expr))
  {}

  void translate(MirFuncBuilder *builder, MirLocal dest) override;

  std::unique_ptr<HirExpr> const_eval(void) override;
  bool const_add(int value) override;
  bool const_mul(int value) override;

private:
  HirUnaryOp op;
  std::unique_ptr<HirExpr> expr;
};

class HirBinaryExpr :public HirExpr
{
public:
  HirBinaryExpr(HirBinaryOp op,
      std::unique_ptr<HirExpr> &&lhs, std::unique_ptr<HirExpr> &&rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs))
  {}

  void translate(MirFuncBuilder *builder, MirLocal dest) override;

  std::unique_ptr<HirExpr> const_eval(void) override;
  bool const_add(int value) override;
  bool const_mul(int value) override;

private:
  HirBinaryOp op;
  std::unique_ptr<HirExpr> lhs;
  std::unique_ptr<HirExpr> rhs;
};

class HirCallExpr :public HirExpr
{
public:
  HirCallExpr(Symbol name,
      std::vector<std::unique_ptr<HirExpr>> &&args)
    : name(name), args(std::move(args))
  {}

  void translate(MirFuncBuilder *builder, MirLocal dest) override;

  std::unique_ptr<HirExpr> const_eval(void) override;

private:
  Symbol name;
  std::vector<std::unique_ptr<HirExpr>> args;
};

class HirCond
{
public:
  virtual void translate(
      MirFuncBuilder *builder, MirLabel label_t, MirLabel label_f) = 0;

  virtual std::unique_ptr<HirCond> const_eval(void);

  virtual bool is_literal(void) const;
  virtual bool get_literal(void) const;
};

class HirTrueCond :public HirCond
{
public:
  HirTrueCond(void) {}

  void translate(MirFuncBuilder *builder,
      MirLabel label_t, MirLabel label_f) override;

  bool is_literal(void) const override;
  bool get_literal(void) const override;
};

class HirFalseCond :public HirCond
{
public:
  HirFalseCond(void) {}

  void translate(MirFuncBuilder *builder,
      MirLabel label_t, MirLabel label_f) override;

  bool is_literal(void) const override;
  bool get_literal(void) const override;
};

class HirBinaryCond :public HirCond
{
public:
  HirBinaryCond(HirLogicalOp op,
      std::unique_ptr<HirExpr> &&lhs, std::unique_ptr<HirExpr> &&rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs))
  {}

  void translate(MirFuncBuilder *builder,
      MirLabel label_t, MirLabel label_f) override;

  std::unique_ptr<HirCond> const_eval(void) override;

private:
  HirLogicalOp op;
  std::unique_ptr<HirExpr> lhs;
  std::unique_ptr<HirExpr> rhs;
};

class HirShortcutCond :public HirCond
{
public:
  HirShortcutCond(HirShortcutOp op,
      std::unique_ptr<HirCond> &&lhs, std::unique_ptr<HirCond> &&rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs))
  {}

  void translate(MirFuncBuilder *builder,
      MirLabel label_t, MirLabel label_f) override;

  std::unique_ptr<HirCond> const_eval(void) override;

private:
  HirShortcutOp op;
  std::unique_ptr<HirCond> lhs;
  std::unique_ptr<HirCond> rhs;
};

class HirStmt
{
public:
  virtual void translate(MirFuncBuilder *builder) = 0;

  virtual void const_eval(void) = 0;
};

class HirStoreStmt :public HirStmt
{
public:
  HirStoreStmt(std::unique_ptr<HirExpr> &&addr,
      std::unique_ptr<HirExpr> &&val)
    : addr(std::move(addr)), val(std::move(val))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirExpr> addr;
  std::unique_ptr<HirExpr> val;
};

class HirReturnStmt :public HirStmt
{
public:
  HirReturnStmt(std::unique_ptr<HirExpr> &&expr)
    : expr(std::move(expr))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirExpr> expr;
};

class HirBlockStmt :public HirStmt
{
public:
  HirBlockStmt(std::vector<std::unique_ptr<HirStmt>> &&stmts)
    : stmts(std::move(stmts))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::vector<std::unique_ptr<HirStmt>> stmts;
};

class HirIfStmt :public HirStmt
{
public:
  HirIfStmt(std::unique_ptr<HirCond> &&cond,
      std::unique_ptr<HirBlockStmt> &&if_stmt)
    : cond(std::move(cond)), if_stmt(std::move(if_stmt))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirCond> cond;
  std::unique_ptr<HirBlockStmt> if_stmt;
};

class HirIfElseStmt :public HirStmt
{
public:
  HirIfElseStmt(std::unique_ptr<HirCond> &&cond,
      std::unique_ptr<HirBlockStmt> &&if_stmt,
      std::unique_ptr<HirBlockStmt> &&else_stmt)
    : cond(std::move(cond)), if_stmt(std::move(if_stmt)),
      else_stmt(std::move(else_stmt))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirCond> cond;
  std::unique_ptr<HirBlockStmt> if_stmt;
  std::unique_ptr<HirBlockStmt> else_stmt;
};

class HirWhileStmt :public HirStmt
{
public:
  HirWhileStmt(std::unique_ptr<HirCond> &&cond,
      std::unique_ptr<HirBlockStmt> &&body)
    : cond(std::move(cond)), body(std::move(body))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirCond> cond;
  std::unique_ptr<HirBlockStmt> body;
};

class HirExprStmt :public HirStmt
{
public:
  HirExprStmt(std::unique_ptr<HirExpr> &&expr)
    : expr(std::move(expr))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirExpr> expr;
};

class HirAssignStmt :public HirStmt
{
public:
  HirAssignStmt(HirLocalId lhs, std::unique_ptr<HirExpr> &&rhs)
    : lhs(lhs), rhs(std::move(rhs))
  {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;

private:
  HirLocalId lhs;
  std::unique_ptr<HirExpr> rhs;
};

class HirContinueStmt :public HirStmt
{
public:
  HirContinueStmt(void) {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;
};

class HirBreakStmt :public HirStmt
{
public:
  HirBreakStmt(void) {}

  void translate(MirFuncBuilder *builder) override;

  void const_eval(void) override;
};

class HirItem
{
public:
  virtual void translate(MirBuilder *builder) = 0;

  virtual void const_eval(void);
};

class HirFuncItem :public HirItem
{
public:
  HirFuncItem(HirFuncBuilder &&builder, Symbol name);

  void translate(MirBuilder *builder) override;

  void const_eval(void) override;

private:
  std::unique_ptr<HirBlockStmt> body;

  size_t num_args;
  size_t num_locals;
  size_t array_sz;
  std::vector<size_t> array_off;

  std::vector<std::unique_ptr<HirItem>> items;

  Symbol name;
};

class HirDataItem :public HirItem
{
public:
  HirDataItem(Symbol name, unsigned int size,
      std::vector<std::pair<unsigned int, int>> &&values)
    : name(name), size(size), values(std::move(values))
  {}

  void translate(MirBuilder *builder) override;

private:
  Symbol name;
  unsigned int size;
  std::vector<std::pair<unsigned int, int>> values;
};

class HirRodataItem :public HirItem
{
public:
  HirRodataItem(Symbol name, unsigned int size,
      std::vector<std::pair<unsigned int, int>> &&values)
    : name(name), size(size), values(std::move(values))
  {}

  void translate(MirBuilder *builder) override;

private:
  Symbol name;
  unsigned int size;
  std::vector<std::pair<unsigned int, int>> values;
};

class HirBssItem :public HirItem
{
public:
  HirBssItem(Symbol name, unsigned int size)
    : name(name), size(size)
  {}

  void translate(MirBuilder *builder) override;

private:
  Symbol name;
  unsigned int size;
};

class HirCompUnit
{
public:
  HirCompUnit(std::vector<std::unique_ptr<HirItem>> &&items)
    : items(std::move(items))
  {}

  std::unique_ptr<MirCompUnit> translate(void);

  void const_eval(void);

private:
  std::vector<std::unique_ptr<HirItem>> items;
};
