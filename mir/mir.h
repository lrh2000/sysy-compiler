#pragma once
#include <vector>
#include <memory>
#include <utility>
#include "defid.h"
#include "../lexer/symbol.h"

enum class MirBinaryOp
{
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Lt,
};

enum class MirImmOp
{
  Add,
  Mul,
  Lt,
};

enum class MirUnaryOp
{
  Neg,
  Nop,
  Eqz,
  Nez,
};

enum class MirLogicalOp
{
  Lt,
  Leq,
  Eq,
  Ne,
};

class MirFuncBuilder;
class MirBuilder;

class MirFuncContext;

class MirStmt
{
public:
  virtual std::vector<unsigned int>
  get_next(const MirFuncContext *ctx, unsigned int id) const;
  virtual MirLocal get_def(void) const;
  virtual std::vector<MirLocal> get_uses(void) const;

  virtual bool is_func_call(void) const;

  virtual void codegen(const MirFuncContext *ctx, unsigned int id) const = 0;
};

class MirEmptyStmt :public MirStmt
{
public:
  MirEmptyStmt(void) {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;
};

class MirSymbolAddrStmt :public MirStmt
{
public:
  MirSymbolAddrStmt(MirLocal dest, Symbol name, off_t offset)
    : dest(dest), name(name), offset(offset)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;

private:
  MirLocal dest;
  Symbol name;
  off_t offset;
};

class MirArrayAddrStmt :public MirStmt
{
public:
  MirArrayAddrStmt(MirLocal dest, MirArray id, off_t offset)
    : dest(dest), id(id), offset(offset)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;

private:
  MirLocal dest;
  MirArray id;
  off_t offset;
};

class MirImmStmt :public MirStmt
{
public:
  MirImmStmt(MirLocal dest, int value)
    : dest(dest), value(value)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;

private:
  MirLocal dest;
  int value;
};

class MirBinaryStmt :public MirStmt
{
public:
  MirBinaryStmt(MirLocal dest,
      MirLocal src1, MirLocal src2, MirBinaryOp op)
    : dest(dest), src1(src1), src2(src2), op(op)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;
  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal dest;
  MirLocal src1;
  MirLocal src2;
  MirBinaryOp op;
};

class MirBinaryImmStmt :public MirStmt
{
public:
  MirBinaryImmStmt(MirLocal dest,
      MirLocal src1, int src2, MirImmOp op)
    : dest(dest), src1(src1), src2(src2), op(op)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;
  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal dest;
  MirLocal src1;
  int src2;
  MirImmOp op;
};

class MirUnaryStmt :public MirStmt
{
public:
  MirUnaryStmt(MirLocal dest, MirLocal src, MirUnaryOp op)
    : dest(dest), src(src), op(op)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;
  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal dest;
  MirLocal src;
  MirUnaryOp op;
};

class MirCallStmt :public MirStmt
{
public:
  MirCallStmt(MirLocal dest,
      Symbol name, std::vector<MirLocal> &&args)
    : dest(dest), name(name), args(std::move(args))
  {}

  bool is_func_call(void) const override;

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;
  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal dest;
  Symbol name;
  std::vector<MirLocal> args;
};

class MirBranchStmt :public MirStmt
{
public:
  MirBranchStmt(MirLocal src1,
      MirLocal src2, MirLabel target, MirLogicalOp op)
    : src1(src1), src2(src2), target(target), op(op)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  std::vector<unsigned int>
  get_next(const MirFuncContext *ctx, unsigned int id) const override;

  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal src1;
  MirLocal src2;
  MirLabel target;
  MirLogicalOp op;
};

class MirJumpStmt :public MirStmt
{
public:
  MirJumpStmt(MirLabel target)
    : target(target)
  {}

  std::vector<unsigned int>
  get_next(const MirFuncContext *ctx, unsigned int id) const override;

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

private:
  MirLabel target;
};

class MirStoreStmt :public MirStmt
{
public:
  MirStoreStmt(MirLocal value, MirLocal address, off_t offset)
    : value(value), address(address), offset(offset)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal value;
  MirLocal address;
  off_t offset;
};

class MirLoadStmt :public MirStmt
{
public:
  MirLoadStmt(MirLocal dest, MirLocal address, off_t offset)
    : dest(dest), address(address), offset(offset)
  {}

  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  MirLocal get_def(void) const override;
  std::vector<MirLocal> get_uses(void) const override;

private:
  MirLocal dest;
  MirLocal address;
  off_t offset;
};

class MirReturnStmt :public MirStmt
{
public:
  MirReturnStmt(void)
    : has_value(false), value(0)
  {}

  MirReturnStmt(MirLocal value)
    : has_value(true), value(value)
  {}

  std::vector<unsigned int>
  get_next(const MirFuncContext *ctx, unsigned int id) const override;
  void codegen(const MirFuncContext *ctx, unsigned int id) const override;

  std::vector<MirLocal> get_uses(void) const override;

private:
  bool has_value;
  MirLocal value;
};

class MirItem
{
public:
  virtual void codegen(void) = 0;
};

class MirFuncItem :public MirItem
{
public:
  MirFuncItem(MirFuncBuilder &&builder);

  void codegen(void) override;

private:
  Symbol name;

  std::vector<size_t> labels;
  std::vector<std::unique_ptr<MirStmt>> stmts;

  size_t num_args;
  size_t num_locals;
  size_t num_temps;

  size_t array_size;
  std::vector<size_t> array_offs;

  friend class MirFuncContext;
};

class MirDataItem :public MirItem
{
public:
  MirDataItem(Symbol name, unsigned int size,
      std::vector<std::pair<unsigned int, int>> &&values)
    : name(name), size(size), values(std::move(values))
  {}

  void codegen(void) override;

private:
  Symbol name;
  unsigned int size;
  std::vector<std::pair<unsigned int, int>> values;
};

class MirRodataItem :public MirItem
{
public:
  MirRodataItem(Symbol name, unsigned int size,
      std::vector<std::pair<unsigned int, int>> &&values)
    : name(name), size(size), values(std::move(values))
  {}

  void codegen(void) override;

private:
  Symbol name;
  unsigned int size;
  std::vector<std::pair<unsigned int, int>> values;
};

class MirBssItem :public MirItem
{
public:
  MirBssItem(Symbol name, unsigned int size)
    : name(name), size(size)
  {}

  void codegen(void) override;

private:
  Symbol name;
  unsigned int size;
};

class MirCompUnit
{
public:
  MirCompUnit(MirBuilder &&builder);

  void codegen(void);

private:
  std::vector<std::unique_ptr<MirItem>> items;
};
