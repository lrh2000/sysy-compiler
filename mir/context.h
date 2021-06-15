#pragma once
#include <utility>
#include <vector>
#include <ostream>
#include <unordered_map>
#include <cassert>
#include <memory>
#include "defid.h"
#include "mir.h"
#include "../asm/register.h"

typedef std::pair<unsigned int, unsigned int> MirOperand;
typedef std::vector<MirOperand> MirOperands;

struct MirLocalLiveness;
struct MirLoop;
struct MirStmtInfo;

class AsmBuilder;
class Bitset;
class MirFuncContext;

class MirSpillOp
{
public:
  virtual void codegen(const MirFuncContext *ctx) const = 0;
};

class MirSpillLoad :public MirSpillOp
{
public:
  MirSpillLoad(Register rd, MirLocal var)
    : rd(rd), var(var)
  {}

  void codegen(const MirFuncContext *ctx) const override;

private:
  Register rd;
  MirLocal var;
};

class MirSpillStore :public MirSpillOp
{
public:
  MirSpillStore(Register rs, MirLocal var)
    : rs(rs), var(var)
  {}

  void codegen(const MirFuncContext *ctx) const override;

private:
  Register rs;
  MirLocal var;
};

class MirRematImm :public MirSpillOp
{
public:
  MirRematImm(Register rd, ssize_t imm)
    : rd(rd), imm(imm)
  {}

  void codegen(const MirFuncContext *ctx) const override;

private:
  Register rd;
  ssize_t imm;
};

class MirRematArrayAddr :public MirSpillOp
{
public:
  MirRematArrayAddr(Register rd, MirArray id, off_t off)
    : rd(rd), id(id), off(off)
  {}

  void codegen(const MirFuncContext *ctx) const override;

private:
  Register rd;
  MirArray id;
  off_t off;
};

class MirRematSymbolAddr :public MirSpillOp
{
public:
  MirRematSymbolAddr(Register rd, Symbol sym, off_t off)
    : rd(rd), sym(sym), off(off)
  {}

  void codegen(const MirFuncContext *ctx) const override;

private:
  Register rd;
  Symbol sym;
  off_t off;
};

class MirFuncContext
{
  typedef std::vector<std::unique_ptr<MirSpillOp>> SpillOps;
  typedef std::unordered_map<unsigned int, SpillOps> SpillPosAndOps;

  typedef std::pair<MirLocal, MirLocal> PhiOp;
  typedef std::vector<PhiOp> PhiOps;
  typedef std::unordered_map<unsigned int, PhiOps> PhiPosAndOps;

public:
  MirFuncContext(MirFuncItem *func, AsmBuilder *builder);
  ~MirFuncContext(void);

  void prepare(void);
  void optimize(void);
  void reg_alloc(void);

  Bitset calc_reachable(void);

  unsigned int label_to_stmt_id(MirLabel label) const
  {
    return func->labels[label];
  }

  MirLabel get_exit_label(void) const
  {
    return func->labels.size() - 1;
  }

  unsigned int get_num_callee_regs(void) const
  {
    return num_callee_regs;
  }

  size_t get_frame_size(void) const
  {
    return sizeof(int) *
      (func->array_size + num_callee_regs + spilled_locals.size());
  }

  off_t get_array_offset(MirArray vid) const
  {
    return sizeof(int) *
      (func->array_offs[vid] + num_callee_regs + spilled_locals.size());
  }

  off_t get_callee_reg_offset(unsigned int rid) const
  {
    return sizeof(int) * (rid + spilled_locals.size());
  }

  off_t get_local_offset(MirLocal vid) const
  {
    auto it = spilled_locals.find(vid);
    assert(it != spilled_locals.end());
    return it->second * sizeof(int);
  }

  const SpillOps &get_spill_loads(unsigned int i) const
  {
    auto it = spill_loads.find(i);
    if (it != spill_loads.end())
      return it->second;
    return g_no_spill_ops;
  }

  const SpillOps &get_spill_stores(unsigned int i) const
  {
    auto it = spill_stores.find(i);
    if (it != spill_stores.end())
      return it->second;
    return g_no_spill_ops;
  }

  Register get_reg(MirOperand operand) const
  {
    return reg_info[operand.first][operand.second];
  }

  AsmBuilder *get_builder(void) const
  {
    return builder;
  }

private:
  bool build_liveness_one(MirLocal local);
  void build_liveness_all(void);

  void spill_liveness_one(
      MirLocalLiveness &ll, std::vector<MirLocalLiveness> &buf);
  void spill_liveness_all(void);

  bool graph_try_color(void);
  void finish_reg_alloc(void);

  void fill_stmt_info(void);
  void fill_defs_and_uses(void);

  void finish_liveness(const MirLocalLiveness &ll, uint32_t color);
  void identify_loops(void);

  Bitset identify_invariants(const MirLoop &loop);
  void move_invariants(void);

  void convert_one_to_ssa(
      MirLocal local, PhiPosAndOps &phi_ops);
  void convert_all_to_ssa(void);

  void merge_duplicates(void);
  void remove_unused(void);

  void spill_regs_cross_func(void);

  MirLocal new_phi(void)
  {
    return num_phis++;
  }

private:
  MirFuncItem *func;

  std::vector<MirStmtInfo> stmt_info;

  std::vector<MirOperands> defs;
  std::vector<MirOperands> uses;

  std::vector<MirLocalLiveness> liveness;
  std::vector<MirLoop> loops;

  std::vector<std::vector<Register>> reg_info;
  std::unordered_map<MirLocal, unsigned int> spilled_locals;
  SpillPosAndOps spill_loads;
  SpillPosAndOps spill_stores;

  unsigned int num_callee_regs;
  mutable AsmBuilder *builder;
  unsigned int num_phis;
  bool tail_reachable;

  static const SpillOps g_no_spill_ops;

  friend struct MirLocalLiveness;
};
