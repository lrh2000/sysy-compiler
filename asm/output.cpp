#include "asm.h"
#include "register.h"

const char *g_register_names[] = {
  "t0", "t1", "t2", "t3", "t4", "t5", "t6",
  "ra",
  "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
  "tp", "gp", "sp",
  "x0",
};

static std::ostream &operator <<(std::ostream &os, AsmLabelId id)
{
  os << ".L" << id.id;
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmBinaryOp op)
{
  switch (op)
  {
  case AsmBinaryOp::Add:
    os << "add";
    break;
  case AsmBinaryOp::Sub:
    os << "sub";
    break;
  case AsmBinaryOp::Mul:
    os << "mul";
    break;
  case AsmBinaryOp::Div:
    os << "div";
    break;
  case AsmBinaryOp::Mod:
    os << "rem";
    break;
  case AsmBinaryOp::Lt:
    os << "slt";
    break;
  }
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmBinaryImmOp op)
{
  switch (op)
  {
  case AsmBinaryImmOp::Add:
    os << "addi";
    break;
  case AsmBinaryImmOp::Shift:
    os << "slli";
    break;
  case AsmBinaryImmOp::Lt:
    os << "slti";
    break;
  }
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmUnaryOp op)
{
  switch (op)
  {
  case AsmUnaryOp::Mv:
    os << "mv";
    break;
  case AsmUnaryOp::Eqz:
    os << "seqz";
    break;
  case AsmUnaryOp::Nez:
    os << "snez";
    break;
  case AsmUnaryOp::Neg:
    os << "neg";
    break;
  case AsmUnaryOp::Jump:
    os << "jr";
    break;
  }
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmMemoryOp op)
{
  switch (op)
  {
  case AsmMemoryOp::Load:
    os << "lw";
    break;
  case AsmMemoryOp::Store:
    os << "sw";
    break;
  }
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmBranchOp op)
{
  switch (op)
  {
  case AsmBranchOp::Lt:
    os << "blt";
    break;
  case AsmBranchOp::Leq:
    os << "ble";
    break;
  case AsmBranchOp::Eq:
    os << "beq";
    break;
  case AsmBranchOp::Ne:
    os << "bne";
    break;
  }
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmLabelSec sec)
{
  switch (sec)
  {
  case AsmLabelSec::Text:
    os << ".text";
    break;
  case AsmLabelSec::Data:
    os << ".data";
    break;
  case AsmLabelSec::Rodata:
    os << ".rodata";
    break;
  case AsmLabelSec::Bss:
    os << ".bss";
    break;
  }
  return os;
}

static std::ostream &operator <<(std::ostream &os, AsmIntDirType dir)
{
  switch (dir)
  {
  case AsmIntDirType::Put:
    os << ".long";
    break;
  case AsmIntDirType::Skip:
    os << ".skip";
    break;
  }
  return os;
}

void AsmGlobalLabel::print(std::ostream &os) const
{
  os << ".section " << section << "\n";
  os << ".globl " << sym.to_string() << "\n";
  os << sym.to_string() << ":\n";
}

void AsmIntDirective::print(std::ostream &os) const
{
  os << "  " << type << " " << data << "\n";
}

void AsmLocalLabel::print(std::ostream &os) const
{
  os << labelid << ":\n";
}

void AsmBinaryInst::print(std::ostream &os) const
{
  os << "  "
     << op
     << " "
     << rd
     << ", "
     << rs1
     << ", "
     << rs2
     << "\n";
}

void AsmBinaryImmInst::print(std::ostream &os) const
{
  assert(rs2 >= -2048 && rs2 <= 2047);
  assert(op != AsmBinaryImmOp::Shift || (rs2 >= 0 && rs2 < 32));
  os << "  "
     << op
     << " "
     << rd
     << ", "
     << rs1
     << ", "
     << rs2
     << "\n";
}

void AsmUnaryInst::print(std::ostream &os) const
{
  os << "  "
     << op
     << " "
     << rd
     << ", "
     << rs
     << "\n";
}

void AsmLoadImmInst::print(std::ostream &os) const
{
  os << "  "
     << "li"
     << " "
     << rd
     << ", "
     << imm
     << "\n";
}

void AsmLoadAddrInst::print(std::ostream &os) const
{
  os << "  "
     << "la"
     << " "
     << rd
     << ", "
     << sym.to_string()
     << (off >= 0 ? "+" : "")
     << off
     << "\n";
}

void AsmMemoryInst::print(std::ostream &os) const
{
  assert(off >= -2047 && off <= 2048);
  os << "  "
     << op
     << " "
     << reg
     << ", "
     << off
     << "("
     << addr
     << ")"
     << "\n";
}

void AsmCallInst::print(std::ostream &os) const
{
  os << "  "
     << "call"
     << " "
     << sym.to_string()
     << "\n";
}

void AsmJumpInst::print(std::ostream &os) const
{
  os << "  "
     << "j"
     << " "
     << target
     << "\n";
}

void AsmBranchInst::print(std::ostream &os) const
{
  os << "  "
     << op
     << " "
     << rs1
     << ", "
     << rs2
     << ", "
     << target
     << "\n";
}

void AsmJumpRegInst::print(std::ostream &os) const
{
  os << "  "
     << "jr"
     << " "
     << rs
     << "\n";
}

std::ostream &operator <<(std::ostream &os, const AsmFile &file)
{
  for (const auto &line : file.lines)
    os << *line;
  return os;
}
