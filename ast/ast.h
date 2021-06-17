#pragma once
#include <vector>
#include <memory>
#include "../lexer/symbol.h"
#include "defid.h"

enum class AstBinaryOp
{
  Add,
  Sub,
  Mul,
  Div,
  Mod,
};

enum class AstUnaryOp
{
  Not,
  Pos,
  Neg,
};

enum class AstLogicalOp
{
  Or,
  And,
  Lt,
  Gt,
  Leq,
  Geq,
  Eq,
  Ne,
};

class HirFuncBuilder;
class HirExpr;
class HirCond;
class HirBlockStmt;
class HirStmt;
class HirItem;
class HirCompUnit;

class AstContext;
class AstType;

class AstExpr
{
public:
  AstExpr(void) {}

  virtual void name_resolve(AstContext *ctx) = 0;
  virtual int const_eval(const AstContext *ctx) const = 0;
  virtual const AstType *type_check(AstContext *ctx) = 0;

  virtual std::unique_ptr<AstExpr> take_if_unary_not(void);

  virtual std::unique_ptr<HirExpr> translate(AstContext *ctx) = 0;
};

class AstBinaryExpr :public AstExpr
{
public:
  AstBinaryExpr(std::unique_ptr<AstExpr> &&lhs,
      std::unique_ptr<AstExpr> &&rhs, AstBinaryOp op)
    : lhs(std::move(lhs)), rhs(std::move(rhs)), op(op)
  {}

  void name_resolve(AstContext *ctx) override;
  int const_eval(const AstContext *ctx) const override;
  const AstType *type_check(AstContext *ctx) override;

  std::unique_ptr<HirExpr> translate(AstContext *ctx) override;

protected:
  std::unique_ptr<AstExpr> lhs;
  std::unique_ptr<AstExpr> rhs;
  AstBinaryOp op;
};

class AstUnaryExpr :public AstExpr
{
public:
  AstUnaryExpr(std::unique_ptr<AstExpr> &&expr, AstUnaryOp op)
    : expr(std::move(expr)), op(op)
  {}

  void name_resolve(AstContext *ctx) override;
  int const_eval(const AstContext *ctx) const override;
  const AstType *type_check(AstContext *ctx) override;

  std::unique_ptr<AstExpr> take_if_unary_not(void) override;

  std::unique_ptr<HirExpr> translate(AstContext *ctx) override;

protected:
  std::unique_ptr<AstExpr> expr;
  AstUnaryOp op;
};

class AstLvalExpr :public AstExpr
{
public:
  AstLvalExpr(Symbol sym,
      std::vector<std::unique_ptr<AstExpr>> &&indices)
    : sym(sym), indices(std::move(indices)), ref()
  {}
  
  void name_resolve(AstContext *ctx) override;
  int const_eval(const AstContext *ctx) const override;
  const AstType *type_check(AstContext *ctx) override;

  std::unique_ptr<HirExpr> into_addr(AstContext *ctx);
  std::unique_ptr<HirStmt>
  assigned_by(AstContext *ctx, std::unique_ptr<HirExpr> &&rhs);

  std::unique_ptr<HirExpr> translate(AstContext *ctx) override;

protected:
  Symbol sym;
  std::vector<std::unique_ptr<AstExpr>> indices;

  AstDefId ref;
};

class AstLiteralExpr :public AstExpr
{
public:
  AstLiteralExpr(Literal literal)
    : literal(literal)
  {}

  void name_resolve(AstContext *ctx) override;
  int const_eval(const AstContext *ctx) const override;
  const AstType *type_check(AstContext *ctx) override;

  std::unique_ptr<HirExpr> translate(AstContext *ctx) override;

protected:
  Literal literal;
};

class AstCallExpr :public AstExpr
{
public:
  AstCallExpr(Symbol sym,
      std::vector<std::unique_ptr<AstExpr>> args)
    : sym(sym), args(std::move(args))
  {}

  void name_resolve(AstContext *ctx) override;
  int const_eval(const AstContext *ctx) const override;
  const AstType *type_check(AstContext *ctx) override;

  std::unique_ptr<HirExpr> translate(AstContext *ctx) override;

protected:
  Symbol sym;
  std::vector<std::unique_ptr<AstExpr>> args;

  AstDefId ref;
};

class AstCond
{
public:
  AstCond(void) {}

  virtual void name_resolve(AstContext *ctx) = 0;
  virtual void type_check(AstContext *ctx) = 0;

  virtual std::unique_ptr<HirCond>
  translate_into_cond(AstContext *ctx) = 0;
  virtual std::unique_ptr<HirExpr>
  translate_into_expr(AstContext *ctx) = 0;
};

class AstExprCond :public AstCond
{
public:
  AstExprCond(std::unique_ptr<AstExpr> &&expr)
    : expr(std::move(expr))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  std::unique_ptr<HirCond>
  translate_into_cond(AstContext *ctx) override;
  std::unique_ptr<HirExpr>
  translate_into_expr(AstContext *ctx) override;

protected:
  std::unique_ptr<AstExpr> expr;
};

class AstBinaryCond :public AstCond
{
public:
  AstBinaryCond(std::unique_ptr<AstCond> &&lhs,
      std::unique_ptr<AstCond> &&rhs, AstLogicalOp op)
    : lhs(std::move(lhs)), rhs(std::move(rhs)), op(op)
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  std::unique_ptr<HirCond>
  translate_into_cond(AstContext *ctx) override;
  std::unique_ptr<HirExpr>
  translate_into_expr(AstContext *ctx) override;

protected:
  std::unique_ptr<AstCond> lhs;
  std::unique_ptr<AstCond> rhs;
  AstLogicalOp op;
};

class AstInit
{
public:
  typedef
    std::vector<std::pair<unsigned int, std::unique_ptr<AstExpr>>>
    ExprVector;
  typedef
    std::vector<std::pair<unsigned int, int>>
    LiteralVector;

  AstInit(bool is_list)
    : is_list_(is_list)
  {}

  bool is_list(void)
  {
    return is_list_;
  }

  virtual void name_resolve(AstContext *ctx) = 0;
  virtual void type_check(AstContext *ctx) = 0;

  ExprVector collect(const std::vector<unsigned int> &shape);
  LiteralVector collect_const(const AstContext *ctx,
      const std::vector<unsigned int> &shape);

protected:
  virtual size_t do_collect(
    size_t position,
    const std::vector<unsigned int> &shape,
    size_t depth,
    ExprVector &result,
    size_t base) = 0;

  virtual void do_collect_all(
    const std::vector<unsigned int> &shape,
    size_t depth,
    ExprVector &result,
    size_t base) = 0;

private:
  bool is_list_;

  friend class AstListInit;
};

class AstExprInit :public AstInit
{
public:
  AstExprInit(std::unique_ptr<AstExpr> &&expr)
    : AstInit(false), expr(std::move(expr))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

protected:
  size_t do_collect(
    size_t position,
    const std::vector<unsigned int> &shape,
    size_t depth,
    ExprVector &result,
    size_t base) override;

  void do_collect_all(
    const std::vector<unsigned int> &shape,
    size_t depth,
    ExprVector &result,
    size_t base) override;

protected:
  std::unique_ptr<AstExpr> expr;
};

class AstListInit :public AstInit
{
public:
  AstListInit(std::vector<std::unique_ptr<AstInit>> &&list)
    : AstInit(true), list(std::move(list))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

protected:
  size_t do_collect(
    size_t position,
    const std::vector<unsigned int> &shape,
    size_t depth,
    ExprVector &result,
    size_t base) override;

  void do_collect_all(
    const std::vector<unsigned int> &shape,
    size_t depth,
    ExprVector &result,
    size_t base) override;

protected:
  std::vector<std::unique_ptr<AstInit>> list;
};

class AstStmt
{
public:
  AstStmt(void) {}

  virtual void name_resolve(AstContext *ctx) = 0;
  virtual void type_check(AstContext *ctx) = 0;

  virtual void translate(AstContext *ctx, HirFuncBuilder *builder) = 0;
};

class AstExprStmt :public AstStmt
{
public:
  AstExprStmt(std::unique_ptr<AstExpr> &&expr)
    : expr(std::move(expr))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::unique_ptr<AstExpr> expr;
};

class AstReturnStmt :public AstStmt
{
public:
  AstReturnStmt(std::unique_ptr<AstExpr> &&expr)
    : expr(std::move(expr))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::unique_ptr<AstExpr> expr;
};

class AstDeclStmt :public AstStmt
{
public:
  AstDeclStmt(bool is_const, std::vector<Symbol> &&sym,
      std::vector<std::vector<std::unique_ptr<AstExpr>>> &&indices,
      std::vector<std::unique_ptr<AstInit>> &&init)
    : is_const(is_const), sym(std::move(sym)),
      indices(std::move(indices)),
      init(std::move(init)), def()
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  bool is_const;
  std::vector<Symbol> sym;
  std::vector<std::vector<std::unique_ptr<AstExpr>>> indices;
  std::vector<std::unique_ptr<AstInit>> init;

  std::vector<AstDefId> def;
};

class AstAssignStmt :public AstStmt
{
public:
  AstAssignStmt(std::unique_ptr<AstLvalExpr> &&lhs,
      std::unique_ptr<AstExpr> &&rhs)
    : lhs(std::move(lhs)), rhs(std::move(rhs))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::unique_ptr<AstLvalExpr> lhs;
  std::unique_ptr<AstExpr> rhs; 
};

class AstBlockStmt :public AstStmt
{
public:
  AstBlockStmt(std::vector<std::unique_ptr<AstStmt>> &&stmts)
    : stmts(std::move(stmts))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  std::unique_ptr<HirBlockStmt>
  translate_into_block(AstContext *ctx, HirFuncBuilder *builder);
  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::vector<std::unique_ptr<AstStmt>> stmts;
};

class AstIfStmt :public AstStmt
{
public:
  AstIfStmt(std::unique_ptr<AstBlockStmt> &&if_stmt,
      std::unique_ptr<AstCond> &&cond)
    : if_stmt(std::move(if_stmt)),
      cond(std::move(cond))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::unique_ptr<AstBlockStmt> if_stmt;
  std::unique_ptr<AstCond> cond;
};

class AstIfElseStmt :public AstStmt
{
public:
  AstIfElseStmt(std::unique_ptr<AstBlockStmt> &&if_stmt,
      std::unique_ptr<AstBlockStmt> &&else_stmt,
      std::unique_ptr<AstCond> &&cond)
    : if_stmt(std::move(if_stmt)),
      else_stmt(std::move(else_stmt)),
      cond(std::move(cond))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::unique_ptr<AstBlockStmt> if_stmt;
  std::unique_ptr<AstBlockStmt> else_stmt;
  std::unique_ptr<AstCond> cond;
};

class AstWhileStmt :public AstStmt
{
public:
  AstWhileStmt(std::unique_ptr<AstBlockStmt> &&body,
      std::unique_ptr<AstCond> &&cond)
    : body(std::move(body)), cond(std::move(cond))
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;

protected:
  std::unique_ptr<AstBlockStmt> body;
  std::unique_ptr<AstCond> cond;
};

class AstBreakStmt :public AstStmt
{
public:
  AstBreakStmt(void) {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;
};

class AstContinueStmt :public AstStmt
{
public:
  AstContinueStmt(void) {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;
};

class AstEmptyStmt :public AstStmt
{
public:
  AstEmptyStmt(void) {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  void translate(AstContext *ctx, HirFuncBuilder *builder) override;
};

class AstFuncArg
{
public:
  AstFuncArg(Symbol sym,
      std::vector<std::unique_ptr<AstExpr>> &&indices)
    : sym(sym), indices(std::move(indices)), def()
  {}

  void name_resolve(AstContext *ctx);
  const AstType *type_check(AstContext *ctx);

  void translate(AstContext *ctx, HirFuncBuilder *builder);

private:
  Symbol sym;
  std::vector<std::unique_ptr<AstExpr>> indices;

  AstDefId def;
};

class AstItem
{
public:
  AstItem(void) {}

  virtual void name_resolve(AstContext *ctx) = 0;
  virtual void type_check(AstContext *ctx) = 0;

  virtual size_t num_items(void) const = 0;
  virtual std::unique_ptr<HirItem>
  translate(AstContext *ctx, size_t i) = 0;
};

class AstFuncItem :public AstItem
{
public:
  AstFuncItem(bool is_void, Symbol sym,
      std::vector<std::unique_ptr<AstFuncArg>> &&args,
      std::vector<std::unique_ptr<AstStmt>> &&body)
    : is_void(is_void), sym(sym),
      args(std::move(args)), body(std::move(body)), def()
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  size_t num_items(void) const override;
  std::unique_ptr<HirItem>
  translate(AstContext *ctx, size_t i) override;

protected:
  bool is_void;
  Symbol sym;
  std::vector<std::unique_ptr<AstFuncArg>> args;
  std::vector<std::unique_ptr<AstStmt>> body;

  AstDefId def;
};

class AstDeclItem :public AstItem
{
public:
  AstDeclItem(bool is_const, std::vector<Symbol> &&sym,
      std::vector<std::vector<std::unique_ptr<AstExpr>>> &&indices,
      std::vector<std::unique_ptr<AstInit>> &&init)
    : is_const(is_const), sym(std::move(sym)),
      indices(std::move(indices)), init(std::move(init)), def()
  {}

  void name_resolve(AstContext *ctx) override;
  void type_check(AstContext *ctx) override;

  size_t num_items(void) const override;
  std::unique_ptr<HirItem>
  translate(AstContext *ctx, size_t i) override;

protected:
  bool is_const;
  std::vector<Symbol> sym;
  std::vector<std::vector<std::unique_ptr<AstExpr>>> indices;
  std::vector<std::unique_ptr<AstInit>> init;

  std::vector<AstDefId> def;
};

class AstCompUnit
{
public:
  AstCompUnit(std::vector<std::unique_ptr<AstItem>> &&items)
    : items(std::move(items))
  {}

  void name_resolve(AstContext *ctx);
  void type_check(AstContext *ctx);

  std::unique_ptr<HirCompUnit> translate(AstContext *ctx);

protected:
  std::vector<std::unique_ptr<AstItem>> items;

  std::vector<AstDefId> prelude_defs;
};
