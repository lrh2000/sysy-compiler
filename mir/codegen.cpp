#include "mir.h"
#include "context.h"
#include "../asm/asm.h"
#include "../asm/builder.h"
#include "../utils/bitset.h"

void MirSpillLoad::codegen(const MirFuncContext *ctx) const
{
  off_t off = ctx->get_local_offset(var);
  ctx->get_builder()->mk_memory_inst(
      AsmMemoryOp::Load, rd, Register::SP, off);
}

void MirSpillStore::codegen(const MirFuncContext *ctx) const
{
  off_t off = ctx->get_local_offset(var);
  ctx->get_builder()->mk_memory_inst(
      AsmMemoryOp::Store, rs, Register::SP, off);
}

void MirRematImm::codegen(const MirFuncContext *ctx) const
{
  ctx->get_builder()->mk_load_imm_inst(rd, imm);
}

void MirRematArrayAddr::codegen(const MirFuncContext *ctx) const
{
  off_t off = ctx->get_array_offset(id);
  off += this->off;
  if (off <= 2047 && off >= -2048) {
    ctx->get_builder()->mk_binary_imm_inst(
        AsmBinaryImmOp::Add, rd, Register::SP, off);
  } else {
    ctx->get_builder()->mk_load_imm_inst(rd, off);
    ctx->get_builder()->mk_binary_inst(
        AsmBinaryOp::Add, rd, rd, Register::SP);
  }
}

void MirRematSymbolAddr::codegen(const MirFuncContext *ctx) const
{
  ctx->get_builder()->mk_load_addr_inst(rd, sym, off);
}

bool MirStmt::is_empty(void) const
{
  return false;
}

bool MirEmptyStmt::is_empty(void) const
{
  return true;
}

bool MirStmt::is_func_call(void) const
{
  return false;
}

bool MirCallStmt::is_func_call(void) const
{
  return true;
}

bool MirStmt::maybe_mem_store(void) const
{
  return false;
}

bool MirCallStmt::maybe_mem_store(void) const
{
  return true;
}

bool MirStoreStmt::maybe_mem_store(void) const
{
  return true;
}

bool MirStmt::is_mem_load(void) const
{
  return false;
}

bool MirLoadStmt::is_mem_load(void) const
{
  return true;
}

bool MirStmt::maybe_jump(void) const
{
  return false;
}

bool MirJumpStmt::maybe_jump(void) const
{
  return true;
}

bool MirBranchStmt::maybe_jump(void) const
{
  return true;
}

bool MirStmt::is_return(void) const
{
  return false;
}

bool MirReturnStmt::is_return(void) const
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
  ctx->get_builder()->mk_load_addr_inst(rd, name, offset);
}

void MirArrayAddrStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));

  off_t offset = this->offset;
  offset += ctx->get_array_offset(this->id);

  if (offset <= 2047 && offset >= -2048) {
    ctx->get_builder()->mk_binary_imm_inst(
        AsmBinaryImmOp::Add, rd, Register::SP, offset);
  } else {
    ctx->get_builder()->mk_load_imm_inst(rd, offset);
    ctx->get_builder()->mk_binary_inst(
        AsmBinaryOp::Add, rd, rd, Register::SP);
  }
}

void MirImmStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  ctx->get_builder()->mk_load_imm_inst(rd, value);
}

void MirBinaryStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));
  Register rs2 = ctx->get_reg(std::make_pair(id, 2u));

  AsmBinaryOp instr;
  switch (op)
  {
  case MirBinaryOp::Add:
    instr = AsmBinaryOp::Add;
    break;
  case MirBinaryOp::Sub:
    instr = AsmBinaryOp::Sub;
    break;
  case MirBinaryOp::Mul:
    instr = AsmBinaryOp::Mul;
    break;
  case MirBinaryOp::Div:
    instr = AsmBinaryOp::Div;
    break;
  case MirBinaryOp::Mod:
    instr = AsmBinaryOp::Mod;
    break;
  case MirBinaryOp::Lt:
    instr = AsmBinaryOp::Lt;
    break;
  }

  ctx->get_builder()->mk_binary_inst(instr, rd, rs1, rs2);
}

void MirBinaryImmStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));

  AsmBinaryImmOp instr;
  unsigned int imm;
  switch (op)
  {
  case MirImmOp::Add:
    instr = AsmBinaryImmOp::Add;
    imm = src2;
    break;
  case MirImmOp::Mul:
    assert(src2 >= 0 && (src2 & (src2 - 1)) == 0);
    instr = AsmBinaryImmOp::Shift;
    imm = __builtin_ctz(src2);
    break;
  case MirImmOp::Lt:
    instr = AsmBinaryImmOp::Lt;
    imm = src2;
    break;
  }

  ctx->get_builder()->mk_binary_imm_inst(instr, rd, rs1, imm);
}

void MirUnaryStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs = ctx->get_reg(std::make_pair(id, 1u));

  AsmUnaryOp instr;
  switch (op)
  {
  case MirUnaryOp::Neg:
    instr = AsmUnaryOp::Neg;
    break;
  case MirUnaryOp::Nop:
    instr = AsmUnaryOp::Mv;
    break;
  case MirUnaryOp::Eqz:
    instr = AsmUnaryOp::Eqz;
    break;
  case MirUnaryOp::Nez:
    instr = AsmUnaryOp::Nez;
    break;
  }

  ctx->get_builder()->mk_unary_inst(instr, rd, rs);
}

void MirCallStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  assert(args.size() <= 8);

  for (unsigned int i = 0; i < args.size(); ++i)
  {
    Register rs = ctx->get_reg(std::make_pair(id, i + 1));
    Register rd = reg_from_arg_id(i + 1);
    assert(static_cast<uint32_t>(rs) < static_cast<uint32_t>(reg_from_arg_id(1))
        || static_cast<uint32_t>(rs) >= static_cast<uint32_t>(rd));
    ctx->get_builder()->mk_unary_inst(AsmUnaryOp::Mv, rd, rs);
  }

  ctx->get_builder()->mk_call_inst(name);

  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  ctx->get_builder()->mk_unary_inst(AsmUnaryOp::Mv, rd, Register::A0);
}

void MirBranchStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));
  Register rs2 = ctx->get_reg(std::make_pair(id, 2u));

  AsmBranchOp instr;
  switch (op)
  {
  case MirLogicalOp::Lt:
    instr = AsmBranchOp::Lt;
    break;
  case MirLogicalOp::Leq:
    instr = AsmBranchOp::Leq;
    break;
  case MirLogicalOp::Eq:
    instr = AsmBranchOp::Eq;
    break;
  case MirLogicalOp::Ne:
    instr = AsmBranchOp::Ne;
    break;
  }

  ctx->get_builder()->mk_branch_inst(instr, rs1, rs2, target);
}

void MirJumpStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  ctx->get_builder()->mk_jump_inst(target);
}

void MirStoreStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rs1 = ctx->get_reg(std::make_pair(id, 1u));
  Register rs2 = ctx->get_reg(std::make_pair(id, 2u));

  ctx->get_builder()->mk_memory_inst(
      AsmMemoryOp::Store, rs1, rs2, offset);
}

void MirLoadStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  Register rd = ctx->get_reg(std::make_pair(id, 0u));
  Register rs = ctx->get_reg(std::make_pair(id, 1u));

  ctx->get_builder()->mk_memory_inst(
      AsmMemoryOp::Load, rd, rs, offset);
}

void MirReturnStmt::codegen(
    const MirFuncContext *ctx, unsigned int id) const
{
  if (has_value) {
    Register rs = ctx->get_reg(std::make_pair(id, 1u));
    ctx->get_builder()->mk_unary_inst(
        AsmUnaryOp::Mv, Register::A0, rs);
  }

  if (ctx->label_to_stmt_id(ctx->get_exit_label()) != id + 1) {
    ctx->get_builder()->mk_jump_inst(ctx->get_exit_label());
  }
}

void MirFuncItem::codegen(AsmBuilder *builder)
{
  assert(num_args <= 9);

  builder->mk_global_label(AsmLabelSec::Text, name);

  MirFuncContext ctx(this, builder);
  ctx.prepare();
  ctx.optimize();
  ctx.reg_alloc();

  builder->alloc_labels(labels.size());

  size_t frame_size = ctx.get_frame_size();
  if (frame_size > 2048) {
    builder->mk_load_imm_inst(Register::T0, frame_size);
    builder->mk_binary_inst(AsmBinaryOp::Sub,
        Register::SP, Register::SP, Register::T0);
  } else if (frame_size > 0) {
    builder->mk_binary_imm_inst(AsmBinaryImmOp::Add,
        Register::SP, Register::SP, -frame_size);
  }

  unsigned int num_callee_regs = ctx.get_num_callee_regs();
  for (unsigned int i = 0; i < num_callee_regs; ++i)
  {
    Register rs = reg_from_callee_id(i);
    builder->mk_memory_inst(
        AsmMemoryOp::Store, rs, Register::SP,
        ctx.get_callee_reg_offset(i));
  }

  for (const auto &store : ctx.get_spill_stores(0))
    store->codegen(&ctx);

  for (unsigned int i = num_args - 1; ~i; --i)
  {
    Register rs = reg_from_arg_id(i);
    Register rd = ctx.get_reg(std::make_pair(0u, i + 1));
    builder->mk_unary_inst(AsmUnaryOp::Mv, rd, rs);
  }

  std::vector<std::pair<unsigned int, MirLabel>> sorted_labels;
  for (size_t i = 0; i < labels.size(); ++i)
    sorted_labels.emplace_back(labels[i], i);
  std::sort(sorted_labels.begin(), sorted_labels.end());

  Bitset reachable = ctx.calc_reachable();

  auto it = sorted_labels.begin();
  for (size_t i = 1; i < stmts.size(); ++i)
  {
    if (it != sorted_labels.end() && it->first == i) {
      if (reachable.get(i))
        builder->mk_local_label(it->second);
      ++it;
    }
    if (!reachable.get(i))
      continue;

    stmts[i]->codegen(&ctx, i);
    for (const auto &store : ctx.get_spill_stores(i))
      store->codegen(&ctx);
    for (const auto &load : ctx.get_spill_loads(i))
      load->codegen(&ctx);
  }

  for (unsigned int i = 0; i < num_callee_regs; ++i)
  {
    Register rs = reg_from_callee_id(i);
    builder->mk_memory_inst(AsmMemoryOp::Load,
        rs, Register::SP, ctx.get_callee_reg_offset(i));
  }

  Register ra = ctx.get_reg(
      std::make_pair((unsigned int) stmts.size() - 1, 1u));

  if (frame_size > 2047) {
    builder->mk_load_imm_inst(Register::T0, frame_size);
    builder->mk_binary_inst(AsmBinaryOp::Add,
        Register::SP, Register::SP, Register::T0);
  } else if (frame_size > 0) {
    builder->mk_binary_imm_inst(AsmBinaryImmOp::Add,
        Register::SP, Register::SP, frame_size);
  }

  builder->mk_jump_reg_inst(ra);
}

void MirDataItem::codegen(AsmBuilder *builder)
{
  builder->mk_global_label(AsmLabelSec::Data, name);

  unsigned int now = 0;
  for (const auto &value : values)
  {
    if (now != value.first) {
      builder->mk_int_directive(
          AsmIntDirType::Skip,
          (value.first - now) * sizeof(int));
    }
    builder->mk_int_directive(
        AsmIntDirType::Put, value.second);
    now = value.first + 1;
  }
  if (now != size) {
    builder->mk_int_directive(
          AsmIntDirType::Skip,
          (size - now) * sizeof(int));
  }
}

void MirRodataItem::codegen(AsmBuilder *builder)
{
  builder->mk_global_label(AsmLabelSec::Rodata, name);

  unsigned int now = 0;
  for (const auto &value : values)
  {
    if (now != value.first) {
      builder->mk_int_directive(
          AsmIntDirType::Skip,
          (value.first - now) * sizeof(int));
    }
    builder->mk_int_directive(
        AsmIntDirType::Put, value.second);
    now = value.first + 1;
  }
  if (now != size) {
    builder->mk_int_directive(
          AsmIntDirType::Skip,
          (size - now) * sizeof(int));
  }
}

void MirBssItem::codegen(AsmBuilder *builder)
{
  builder->mk_global_label(AsmLabelSec::Bss, name);
  builder->mk_int_directive(AsmIntDirType::Skip, size * sizeof(int));
}

std::unique_ptr<AsmFile> MirCompUnit::codegen(void)
{
  AsmBuilder builder;

  for (auto &item : items)
    item->codegen(&builder);

  return std::make_unique<AsmFile>(std::move(builder));
}
