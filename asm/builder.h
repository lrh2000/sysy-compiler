#pragma once
#include <cassert>
#include "asm.h"
#include "../mir/defid.h"

class AsmBuilder
{
public:
  AsmBuilder(void)
    : lines(), label_head(0), label_tail(0)
  {}

  void alloc_labels(unsigned int num)
  {
    label_head = label_tail;
    label_tail += num;
  }

  void mk_global_label(AsmLabelSec section, Symbol sym)
  {
    lines.emplace_back(
        std::make_unique<AsmGlobalLabel>(section, sym));
  }

  void mk_int_directive(AsmIntDirType type, AsmImm data)
  {
    lines.emplace_back(
        std::make_unique<AsmIntDirective>(type, data));
  }

  void mk_local_label(MirLabel mirlabel)
  {
    assert(mirlabel < label_tail - label_head);
    lines.emplace_back(
        std::make_unique<AsmLocalLabel>(
          AsmLabelId(mirlabel + label_head)));
  }

  void mk_binary_inst(AsmBinaryOp op,
      Register rd, Register rs1, Register rs2)
  {
    if (rd == Register::X0)
      return;
    switch (op)
    {
    case AsmBinaryOp::Add:
      if (rd == rs2 && rs1 == Register::X0)
        return;
      /* fallthrough */
    case AsmBinaryOp::Sub:
      if (rd == rs1 && rs2 == Register::X0)
        return;
      break;
    default:
      break;
    }
    lines.emplace_back(
        std::make_unique<AsmBinaryInst>(op, rd, rs1, rs2));
  }

  void mk_binary_imm_inst(AsmBinaryImmOp op,
      Register rd, Register rs1, AsmImm rs2)
  {
    if (rd == Register::X0)
      return;
    switch (op)
    {
    case AsmBinaryImmOp::Add:
    case AsmBinaryImmOp::Shift:
      if (rd == rs1 && rs2 == 0)
        return;
      break;
    default:
      break;
    }
    lines.emplace_back(
        std::make_unique<AsmBinaryImmInst>(op, rd, rs1, rs2));
  }

  void mk_unary_inst(AsmUnaryOp op, Register rd, Register rs)
  {
    if (rd == Register::X0)
      return;
    switch (op)
    {
    case AsmUnaryOp::Mv:
      if (rd == rs)
        return;
      break;
    default:
      break;
    }
    lines.emplace_back(
        std::make_unique<AsmUnaryInst>(op, rd, rs));
  }

  void mk_load_imm_inst(Register rd, AsmImm imm)
  {
    if (rd == Register::X0)
      return;
    lines.emplace_back(
        std::make_unique<AsmLoadImmInst>(rd, imm));
  }

  void mk_load_addr_inst(Register rd,
      Symbol sym, AsmImm off)
  {
    if (rd == Register::X0)
      return;
    lines.emplace_back(
        std::make_unique<AsmLoadAddrInst>(rd, sym, off));
  }

  void mk_memory_inst(AsmMemoryOp op,
      Register reg, Register addr, AsmImm off)
  {
    lines.emplace_back(
        std::make_unique<AsmMemoryInst>(op, reg, addr, off));
  }

  void mk_call_inst(Symbol sym)
  {
    lines.emplace_back(
        std::make_unique<AsmCallInst>(sym));
  }

  void mk_jump_inst(MirLabel mirtarget)
  {
    assert(mirtarget < label_tail - label_head);
    lines.emplace_back(
        std::make_unique<AsmJumpInst>(
          AsmLabelId(mirtarget + label_head)));
  }

  void mk_branch_inst(AsmBranchOp op,
      Register rs1, Register rs2, MirLabel mirtarget)
  {
    assert(mirtarget < label_tail - label_head);

    if (rs1 == rs2) {
      switch (op)
      {
      case AsmBranchOp::Leq:
      case AsmBranchOp::Eq:
        lines.emplace_back(
            std::make_unique<AsmJumpInst>(
              AsmLabelId(mirtarget + label_head)));
        return;
      case AsmBranchOp::Lt:
      case AsmBranchOp::Ne:
        return;
      }
      __builtin_unreachable();
    }

    lines.emplace_back(
        std::make_unique<AsmBranchInst>(op, rs1, rs2,
          AsmLabelId(mirtarget + label_head)));
  }

  void mk_jump_reg_inst(Register rs)
  {
    lines.emplace_back(
        std::make_unique<AsmJumpRegInst>(rs));
  }

private:
  std::vector<std::unique_ptr<AsmLine>> lines;

  MirLabel label_head;
  MirLabel label_tail;

  friend class AsmFile;
};

inline AsmFile::AsmFile(AsmBuilder &&builder)
  : lines(std::move(builder.lines)), num_labels(builder.label_tail)
{}
