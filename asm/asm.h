#pragma once
#include <vector>
#include <memory>
#include <ostream>
#include "defid.h"
#include "register.h"
#include "../lexer/symbol.h"

enum class AsmBinaryOp
{
  Add,
  Sub,
  Div,
  Mul,
  Mod,
  Lt,
};

enum class AsmBinaryImmOp
{
  Add,
  Shift,
  Lt,
};

enum class AsmUnaryOp
{
  Mv,
  Eqz,
  Nez,
  Neg,
  Jump,
};

enum class AsmMemoryOp
{
  Load,
  Store,
};

enum class AsmBranchOp
{
  Lt,
  Leq,
  Eq,
  Ne,
};

enum class AsmLabelSec
{
  Text,
  Data,
  Rodata,
  Bss,
};

enum class AsmIntDirType
{
  Put,
  Skip,
};

class AsmBuilder;

class AsmLine
{
public:
  virtual void print(std::ostream &os) const = 0;
};

class AsmGlobalLabel :public AsmLine
{
public:
  AsmGlobalLabel(AsmLabelSec section, Symbol sym)
    : section(section), sym(sym)
  {}

  void print(std::ostream &os) const override;

private:
  AsmLabelSec section;
  Symbol sym;
};

class AsmIntDirective :public AsmLine
{
public:
  AsmIntDirective(AsmIntDirType type, AsmImm data)
    : type(type), data(data)
  {}

  void print(std::ostream &os) const override;

private:
  AsmIntDirType type;
  AsmImm data;
};

class AsmLocalLabel :public AsmLine
{
public:
  AsmLocalLabel(AsmLabelId labelid)
    : labelid(labelid)
  {}

  void print(std::ostream &os) const override;

private:
  AsmLabelId labelid;
};

class AsmBinaryInst :public AsmLine
{
public:
  AsmBinaryInst(AsmBinaryOp op,
      Register rd, Register rs1, Register rs2)
    : op(op), rd(rd), rs1(rs1), rs2(rs2)
  {}

  void print(std::ostream &os) const override;

private:
  AsmBinaryOp op;
  Register rd;
  Register rs1;
  Register rs2;
};

class AsmBinaryImmInst :public AsmLine
{
public:
  AsmBinaryImmInst(AsmBinaryImmOp op,
      Register rd, Register rs1, AsmImm rs2)
    : op(op), rd(rd), rs1(rs1), rs2(rs2)
  {}

  void print(std::ostream &os) const override;

private:
  AsmBinaryImmOp op;
  Register rd;
  Register rs1;
  AsmImm rs2;
};

class AsmUnaryInst :public AsmLine
{
public:
  AsmUnaryInst(AsmUnaryOp op,
      Register rd, Register rs)
    : op(op), rd(rd), rs(rs)
  {}

  void print(std::ostream &os) const override;

private:
  AsmUnaryOp op;
  Register rd;
  Register rs;
};

class AsmLoadImmInst :public AsmLine
{
public:
  AsmLoadImmInst(Register rd, AsmImm imm)
    : rd(rd), imm(imm)
  {}

  void print(std::ostream &os) const override;

private:
  Register rd;
  AsmImm imm;
};

class AsmLoadAddrInst :public AsmLine
{
public:
  AsmLoadAddrInst(Register rd, Symbol sym, AsmImm off)
    : rd(rd), sym(sym), off(off)
  {}

  void print(std::ostream &os) const override;

private:
  Register rd;
  Symbol sym;
  AsmImm off;
};

class AsmMemoryInst :public AsmLine
{
public:
  AsmMemoryInst(AsmMemoryOp op,
      Register reg, Register addr, AsmImm off)
    : op(op), reg(reg), addr(addr), off(off)
  {}

  void print(std::ostream &os) const override;

private:
  AsmMemoryOp op;
  Register reg;
  Register addr;
  AsmImm off;
};

class AsmCallInst :public AsmLine
{
public:
  AsmCallInst(Symbol sym)
    : sym(sym)
  {}

  void print(std::ostream &os) const override;

private:
  Symbol sym;
};

class AsmJumpInst :public AsmLine
{
public:
  AsmJumpInst(AsmLabelId target)
    : target(target)
  {}

  void print(std::ostream &os) const override;

private:
  AsmLabelId target;
};

class AsmBranchInst :public AsmLine
{
public:
  AsmBranchInst(AsmBranchOp op,
      Register rs1, Register rs2, AsmLabelId target)
    : op(op), rs1(rs1), rs2(rs2), target(target)
  {}

  void print(std::ostream &os) const override;

private:
  AsmBranchOp op;
  Register rs1;
  Register rs2;
  AsmLabelId target;
};

class AsmJumpRegInst :public AsmLine
{
public:
  AsmJumpRegInst(Register rs)
    : rs(rs)
  {}

  void print(std::ostream &os) const override;

private:
  Register rs;
};

extern inline std::ostream &
operator <<(std::ostream &os, const AsmLine &line)
{
  line.print(os);
  return os;
}

class AsmFile
{
public:
  AsmFile(AsmBuilder &&builder);

private:
  std::vector<std::unique_ptr<AsmLine>> lines;

  friend std::ostream &operator <<(std::ostream &os, const AsmFile &file);
};

std::ostream &operator <<(std::ostream &os, const AsmFile &file);
