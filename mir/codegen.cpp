#include <iostream>
#include "mir.h"
#include "context.h"

bool MirStmt::is_func_call(void) const
{
  return false;
}

bool MirCallStmt::is_func_call(void) const
{
  return true;
}

std::vector<unsigned int>
MirStmt::get_next(const MirFuncContext *ctx, unsigned int id) const
{
  return { id + 1 };
}

std::vector<unsigned int>
MirBranchStmt::get_next(const MirFuncContext *ctx, unsigned int id) const
{
  return { id + 1, ctx->label_to_stmt_id(target) };
}

std::vector<unsigned int>
MirJumpStmt::get_next(const MirFuncContext *ctx, unsigned int id) const
{
  return { ctx->label_to_stmt_id(target) };
}

std::vector<unsigned int>
MirReturnStmt::get_next(const MirFuncContext *ctx, unsigned int id) const
{
  return { ctx->label_to_stmt_id(ctx->get_exit_label()) };
}

MirLocal MirStmt::get_def(void) const
{
  return ~0u;
}

MirLocal MirSymbolAddrStmt::get_def(void) const
{
  return dest;
}

MirLocal MirArrayAddrStmt::get_def(void) const
{
  return dest;
}

MirLocal MirImmStmt::get_def(void) const
{
  return dest;
}

MirLocal MirBinaryStmt::get_def(void) const
{
  return dest;
}

MirLocal MirBinaryImmStmt::get_def(void) const
{
  return dest;
}

MirLocal MirUnaryStmt::get_def(void) const
{
  return dest;
}

MirLocal MirCallStmt::get_def(void) const
{
  return dest;
}

MirLocal MirLoadStmt::get_def(void) const
{
  return dest;
}

std::vector<MirLocal> MirStmt::get_uses(void) const
{
  return {};
}

std::vector<MirLocal> MirBinaryStmt::get_uses(void) const
{
  return { src1, src2 };
}

std::vector<MirLocal> MirBinaryImmStmt::get_uses(void) const
{
  return { src1 };
}

std::vector<MirLocal> MirUnaryStmt::get_uses(void) const
{
  return { src };
}

std::vector<MirLocal> MirCallStmt::get_uses(void) const
{
  return args;
}

std::vector<MirLocal> MirBranchStmt::get_uses(void) const
{
  return { src1, src2 };
}

std::vector<MirLocal> MirStoreStmt::get_uses(void) const
{
  return { value, address };
}

std::vector<MirLocal> MirLoadStmt::get_uses(void) const
{
  return { address };
}

std::vector<MirLocal> MirReturnStmt::get_uses(void) const
{
  if (has_value)
    return { value };
  return {};
}

void MirEmptyStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{ /* nothing */ }

void MirSymbolAddrStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));

  std::cout << "  la "
            << rd
            << ", "
            << name.to_string()
            << (offset >= 0 ? "+" : "")
            << offset
            << "\n";
}

void MirArrayAddrStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));

  off_t offset = this->offset;
  offset += ctx->get_array_offset(this->id);

  if (offset <= 2047 && offset >= -2048) {
    std::cout << "  addi "
              << rd
              << ", sp, "
              << offset
              << "\n";
  } else {
    std::cout << "  li "
              << rd
              << ", "
              << offset
              << "\n";
    std::cout << "  add "
              << rd
              << ", "
              << rd
              << ", sp\n";
  }
}

void MirImmStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));

  std::cout << "  li "
            << rd
            << ", "
            << value
            << "\n";
}

void MirBinaryStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));
  Register rs2 = ctx->get_reg(std::make_pair(id, 2u));

  std::string instr;
  switch (op)
  {
  case MirBinaryOp::Add:
    instr = "add";
    break;
  case MirBinaryOp::Sub:
    instr = "sub";
    break;
  case MirBinaryOp::Mul:
    instr = "mul";
    break;
  case MirBinaryOp::Div:
    instr = "div";
    break;
  case MirBinaryOp::Mod:
    instr = "rem";
    break;
  case MirBinaryOp::Lt:
    instr = "slt";
    break;
  }

  std::cout << "  "
            << instr
            << " "
            << rd
            << ", "
            << rs1
            << ", "
            << rs2
            << "\n";
}

void MirBinaryImmStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));

  std::string instr;
  unsigned int imm;
  switch (op)
  {
  case MirImmOp::Add:
    assert(src2 <= 2047 && src2 >= -2048);
    instr = "addi";
    imm = src2;
    break;
  case MirImmOp::Mul:
    assert(src2 >= 0 && (src2 & (src2 - 1)) == 0);
    instr = "slli";
    imm = __builtin_ctz(src2);
    break;
  case MirImmOp::Lt:
    assert(src2 <= 2047 && src2 >= -2048);
    instr = "slti";
    imm = src2;
    break;
  }

  std::cout << "  "
            << instr
            << " "
            << rd
            << ", "
            << rs1
            << ", "
            << imm
            << "\n";
}

void MirUnaryStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs = ctx->get_reg(std::make_pair(id, 1u));

  std::string instr;
  switch (op)
  {
  case MirUnaryOp::Neg:
    instr = "neg";
    break;
  case MirUnaryOp::Nop:
    instr = "mv";
    break;
  case MirUnaryOp::Eqz:
    instr = "seqz";
    break;
  case MirUnaryOp::Nez:
    instr = "snez";
    break;
  }

  std::cout << "  "
            << instr
            << " "
            << rd
            << ", "
            << rs
            << "\n";
}

void MirCallStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  assert(args.size() <= 8);

  for (unsigned int i = 0; i < args.size(); ++i)
  {
    Register rs = ctx->get_reg(std::make_pair(i, id));
    Register rd = static_cast<Register>(i + 1);
    if (rs != rd) {
      std::cout << "  mv "
                << rd
                << ", "
                << rs
                << "\n";
    }
  }

  std::cout << "  call "
            << name.to_string()
            << "\n";

  if (dest != ~0u) {
    Register rd = ctx->get_reg(std::make_pair(id, 0u));
    std::cout << "  mv "
              << rd
              << ", a0\n";
  }
}

void MirBranchStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));
  Register rs2 = ctx->get_reg(std::make_pair(id, 2u));

  std::string instr;
  switch (op)
  {
  case MirLogicalOp::Lt:
    instr = "blt";
    break;
  case MirLogicalOp::Leq:
    instr = "bleq";
    break;
  case MirLogicalOp::Eq:
    instr = "beq";
    break;
  case MirLogicalOp::Ne:
    instr = "bne";
    break;
  }

  std::cout << "  "
            << instr
            << " "
            << rs1
            << ", "
            << rs2
            << ", .L"
            << target
            << "\n";
}

void MirJumpStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  std::cout << "j .L"
            << target
            << "\n";
}

void MirStoreStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));
  Register rs2 = ctx->get_reg(std::make_pair(id, 2u));

  std::cout << "sw "
            << rs1
            << ", "
            << offset
            << "("
            << rs2
            << ")\n";
}

void MirLoadStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs = ctx->get_reg(std::make_pair(id, 1u));

  std::cout << "lw "
            << rd
            << ", "
            << offset
            << "("
            << rs
            << ")\n";
}

void MirReturnStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  if (has_value) {
    Register rs = ctx->get_reg(std::make_pair(id, 0u));
    if (rs != Register::A0) {
      std::cout << "mv a0, "
                << rs
                << "\n";
    }
  }

  if (ctx->label_to_stmt_id(ctx->get_exit_label()) != id + 1) {
    std::cout << "j .L"
              << ctx->get_exit_label()
              << "\n";
  }
}

void MirFuncItem::codegen(void)
{
  assert(num_args <= 9);

  std::cout << ".text\n"
            << name.to_string() << ":\n";

  MirFuncContext ctx(this);
  ctx.prepare();
  ctx.reg_alloc();

  size_t frame_size = ctx.get_frame_size();
  if (frame_size <= 2048) {
    std::cout << "  addi sp, sp, -"
              << frame_size
              << "\n";
  } else {
    std::cout << "  li t0, "
              << frame_size
              << "\n";
    std::cout << "  addi sp, sp, t0\n";
  }

  unsigned int num_callee_regs = ctx.get_num_callee_regs();
  for (unsigned int i = 0; i < num_callee_regs; ++i)
  {
    unsigned int regid = i + NR_REG_CALLER;
    Register rs = static_cast<Register>(regid);
    std::cout << "  sw "
              << rs
              << ", "
              << ctx.get_callee_reg_offset(i)
              << "(sp)\n";
  }

  for (const auto &store : ctx.get_spill_stores(0))
  {
    assert(store.first < num_args);
    std::cout << "  sw "
              << store.second
              << ", "
              << ctx.get_local_offset(store.first)
              << "(sp)\n";
  }

  for (unsigned int i = num_args - 1; ~i; --i)
  {
    Register rs = static_cast<Register>(i);
    Register rd = ctx.get_reg(std::make_pair(0u, i + 1));
    if (rs != rd) {
      std::cout << "  mv "
                << rd
                << ", "
                << rs
                << "\n";
    }
  }

  std::vector<std::pair<unsigned int, MirLabel>> sorted_labels;
  for (size_t i = 0; i < labels.size(); ++i)
    sorted_labels.emplace_back(labels[i], i);
  std::sort(sorted_labels.begin(), sorted_labels.end());

  auto it = sorted_labels.begin();
  for (size_t i = 1; i < stmts.size(); ++i)
  {
    if (it != sorted_labels.end() && it->first == i) {
      std::cout << ".L"
                << it->second
                << ":\n";
      ++it;
    }
    stmts[i]->codegen(&ctx, i);
  }

  for (unsigned int i = 0; i < num_callee_regs; ++i)
  {
    unsigned int regid = i + NR_REG_CALLER;
    Register rs = static_cast<Register>(regid);
    std::cout << "  lw "
              << rs
              << ", "
              << ctx.get_callee_reg_offset(i)
              << "(sp)\n";
  }

  Register ra = ctx.get_reg(
      std::make_pair((unsigned int) stmts.size() - 1, 1u));

  if (frame_size <= 2047) {
    std::cout << "  addi sp, sp, "
              << frame_size
              << "\n";
  } else {
    Register tmp =
      ra == Register::T0 ? Register::T1 : Register::T0;
    std::cout << "  li "
              << tmp
              << ", "
              << frame_size
              << "\n";
    std::cout << "  addi sp, sp, "
              << tmp
              << "\n";
  }

  std::cout << "  jr "
            << ra
            << "\n";

  std::cout << std::endl;
}

void MirDataItem::codegen(void)
{
  std::cout << ".data\n"
            << name.to_string() << ":\n";

  unsigned int now = 0;
  for (const auto &value : values)
  {
    if (now != value.first) {
      std::cout << ".skip "
                << (value.first - now) * sizeof(int)
                << "\n";
    }
    std::cout << ".long " << value.second << "\n";
    now = value.first + 1;
  }
  if (now != size) {
    std::cout << ".skip "
              << (size - now) * sizeof(int)
              << "\n";
  }

  std::cout << std::endl;
}

void MirRodataItem::codegen(void)
{
  std::cout << ".rodata\n"
            << name.to_string() << ":\n";

  unsigned int now = 0;
  for (const auto &value : values)
  {
    if (now != value.first) {
      std::cout << ".skip "
                << (value.first - now) * sizeof(int)
                << "\n";
    }
    std::cout << ".long " << value.second << "\n";
    now = value.first + 1;
  }
  if (now != size) {
    std::cout << ".skip "
              << (size - now) * sizeof(int)
              << "\n";
  }

  std::cout << std::endl;
}

void MirBssItem::codegen(void)
{
  std::cout << ".bss\n"
            << name.to_string() << ":\n"
            << "  .skip " << size * sizeof(int) << "\n"
            << std::endl;
}

void MirCompUnit::codegen(void)
{
  for (auto &item : items)
    item->codegen();
}
