#pragma once
#include <utility>
#include <vector>
#include <ostream>
#include <unordered_map>
#include <cassert>
#include "defid.h"
#include "mir.h"

typedef std::pair<unsigned int, unsigned int> MirOperand;
typedef std::vector<MirOperand> MirOperands;

#define NR_REG_CALLER    16
#define NR_REG_CALLEE    14
#define NR_REGISTERS     30

#define MASK_REG_CALLER  0x0000ffff
#define MASK_REG_CALLEE  0x3fff0000
#define MASK_REGISTERS   0x3fffffff

enum class Register
{
  RA,
  A0, A1, A2, A3, A4, A5, A6, A7,
  T0, T1, T2, T3, T4, T5, T6,
  S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
  GP, TP, SP,
  X0,
  UND,
};

extern const char *g_register_names[];

extern inline std::ostream &operator <<(std::ostream &os, Register reg)
{
  uint32_t regid = static_cast<uint32_t>(reg);
  assert(regid < static_cast<uint32_t>(Register::UND));
  os << g_register_names[regid];
  return os;
}

struct MirLocalLiveness;
struct MirLoop;
struct MirStmtInfo;

class MirFuncContext
{
  typedef std::pair<MirLocal, Register> SpillOp;
  typedef std::vector<SpillOp> SpillOps;
  typedef std::unordered_map<unsigned int, SpillOps> SpillPosAndOps;

public:
  MirFuncContext(MirFuncItem *func);
  ~MirFuncContext(void);

  void prepare(void);
  void reg_alloc(void);

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
    return func->array_size + num_callee_regs * 4 +
      + spilled_locals.size() * sizeof(int);
  }

  off_t get_array_offset(MirArray vid) const
  {
    return func->array_offs[vid] + num_callee_regs * 4 +
      + spilled_locals.size() * sizeof(int);
  }

  off_t get_callee_reg_offset(unsigned int rid) const
  {
    return rid * 4 + spilled_locals.size() * sizeof(int);
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

private:
  void build_liveness_one(MirLocal local);
  void build_liveness_all(void);

  void spill_liveness_one(const MirLocalLiveness &ll);
  void spill_liveness_all(void);

  bool graph_try_color(void);
  void finish_reg_alloc(void);

  void fill_stmt_info(void);
  void fill_defs_and_uses(void);

  void finish_liveness(const MirLocalLiveness &ll);
  void identify_loops(void);

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

  static const SpillOps g_no_spill_ops;
};
