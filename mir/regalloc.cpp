#include <queue>
#include <stack>
#include "mir.h"
#include "context.h"
#include "context_impl.h"
#include "../asm/register.h"
#include "../utils/bitset.h"
#include "../utils/graph.h"

bool MirStmt::can_rematerialize(void) const
{
  return false;
}

std::unique_ptr<MirSpillOp>
MirStmt::rematerialize(Register rd) const
{
  abort();
}

bool MirSymbolAddrStmt::can_rematerialize(void) const
{
  return true;
}

std::unique_ptr<MirSpillOp>
MirSymbolAddrStmt::rematerialize(Register rd) const
{
  return std::make_unique<MirRematSymbolAddr>(rd, name, offset);
}

bool MirArrayAddrStmt::can_rematerialize(void) const
{
  return true;
}

std::unique_ptr<MirSpillOp>
MirArrayAddrStmt::rematerialize(Register rd) const
{
  return std::make_unique<MirRematArrayAddr>(rd, id, offset);
}

bool MirImmStmt::can_rematerialize(void) const
{
  return true;
}

std::unique_ptr<MirSpillOp> MirImmStmt::rematerialize(Register rd) const
{
  return std::make_unique<MirRematImm>(rd, value);
}

bool MirStmt::extract_if_assign(std::pair<MirLocal, MirLocal> &eq) const
{
  return false;
}

bool MirUnaryStmt::extract_if_assign(std::pair<MirLocal, MirLocal> &eq) const
{
  if (op == MirUnaryOp::Nop) {
    eq.first = dest;
    eq.second = src;
    return true;
  }
  return false;
}

const MirFuncContext::SpillOps MirFuncContext::g_no_spill_ops = {};

struct MirLocalLiveness
{
  MirLocalLiveness(Bitset &&stmts,
      std::vector<MirLocalLiveness> &&kids,
      uint32_t hint, uint32_t forbid)
    : stmts(std::move(stmts)), local(~0u), kids(std::move(kids)),
      loop(0), hint(hint), forbid(forbid), to_spill(false),
      color(0), uses(), defs(), remat(nullptr)
  {}

  MirLocalLiveness(Bitset &&stmts,
      MirLocal local, uint32_t hint,
      uint32_t forbid, MirStmt *remat)
    : stmts(std::move(stmts)), local(local), kids(),
      loop(0), hint(hint), forbid(forbid), to_spill(false),
      color(0), uses(), defs(), remat(remat)
  {}

  MirLocalLiveness(Bitset &&stmts, MirLocal local,
      unsigned int loop, uint32_t hint, uint32_t forbid,
      MirOperands &&uses, MirOperands &&defs, MirStmt *remat)
    : stmts(std::move(stmts)), local(local), kids(), loop(loop),
      hint(hint), forbid(forbid), to_spill(false), color(0),
      uses(std::move(uses)), defs(std::move(defs)), remat(remat)
  {}

  MirLocalLiveness(MirLocalLiveness &&other)
    : stmts(std::move(other.stmts)), local(other.local),
      kids(std::move(other.kids)), loop(other.loop),
      hint(other.hint), forbid(other.forbid),
      to_spill(other.to_spill), color(other.color),
      uses(std::move(other.uses)), defs(std::move(other.defs)),
      remat(other.remat)
  {}

  MirLocalLiveness &operator =(MirLocalLiveness &&other)
  {
    stmts = std::move(other.stmts);
    local = other.local;
    kids = std::move(other.kids);
    loop = other.loop;
    hint = other.hint;
    forbid = other.forbid;
    to_spill = other.to_spill;
    color = other.color;
    uses = std::move(other.uses);
    defs = std::move(other.defs);
    remat = other.remat;
    return *this;
  }

  const MirOperands &get_uses(const MirFuncContext *ctx) const
  {
    if (loop > 0)
      return uses;
    else
      return ctx->uses[local];
  }

  const MirOperands &get_defs(const MirFuncContext *ctx) const
  {
    if (loop > 0)
      return defs;
    else
      return ctx->defs[local];
  }

  Bitset stmts;
  MirLocal local;
  std::vector<MirLocalLiveness> kids;
  unsigned int loop;
  uint32_t hint;
  uint32_t forbid;
  bool to_spill;
  uint32_t color;
  MirOperands uses;
  MirOperands defs;
  MirStmt *remat;
};

static inline uint32_t reg_hint_callee_arg(uint32_t i)
{
  return 1u << (static_cast<uint32_t>(Register::RA) + i);
}

static inline uint32_t reg_hint_caller_arg(uint32_t i)
{
  return 1u << (static_cast<uint32_t>(Register::A0) + i);
}

static inline uint32_t reg_forbid_callee_arg(uint32_t i)
{
  return (1u << (static_cast<uint32_t>(Register::RA) + i))
    - (1u << static_cast<uint32_t>(Register::RA));
}

static inline uint32_t reg_forbid_caller_arg(uint32_t i)
{
  return (1u << (static_cast<uint32_t>(Register::A0) + i))
    - (1u << static_cast<uint32_t>(Register::A0));
}

static inline uint32_t reg_hint_cross_func(void)
{
  return MASK_REG_CALLEE;
}

static inline uint32_t reg_hint_return_addr(void)
{
  return 1u << static_cast<uint32_t>(Register::RA);
}

static inline uint32_t reg_forbid_return_val(void)
{
  return 0;
}

static inline uint32_t reg_forbid_cross_func(void)
{
  return 0;
}

static inline uint32_t reg_forbid_return_addr(void)
{
  return MASK_REG_CALLEE | (1u << static_cast<uint32_t>(Register::A0));
}

static inline uint32_t reg_hint_return_val(void)
{
  return 1u << static_cast<uint32_t>(Register::A0);
}

MirFuncContext::MirFuncContext(
    MirFuncItem *func, AsmBuilder *builder)
  : func(func), stmt_info(), defs(), uses(),
    liveness(), loops(), reg_info(), spilled_locals(),
    spill_loads(), spill_stores(), num_callee_regs(0),
    builder(builder), num_phis(func->num_temps),
    tail_reachable(true)
{}

MirFuncContext::~MirFuncContext(void)
{}

void MirFuncContext::fill_stmt_info(void)
{
  for (size_t i = 0; i < func->stmts.size(); ++i)
  {
    auto next = func->stmts[i]->get_next(this, i);
    MirLocal def = func->stmts[i]->get_def();
    bool func_call = func->stmts[i]->is_func_call();

    stmt_info.emplace_back(std::move(next),
        std::vector<unsigned int>(), def, func_call);
  }
  stmt_info[stmt_info.size() - 1].next.clear();

  for (size_t i = 0; i < stmt_info.size(); ++i)
    for (auto npos : stmt_info[i].next)
      stmt_info[npos].prev.emplace_back(i);
}

void MirFuncContext::fill_defs_and_uses(void)
{
  defs.resize(num_phis);
  uses.resize(num_phis);

  reg_info.resize(stmt_info.size());

  for (size_t i = 0; i < func->num_args; ++i)
    defs[i].emplace_back(0, i + 1);
  reg_info[0].resize(func->num_args + 1, Register::UND);

  for (size_t i = 1; i < func->stmts.size() - 1; ++i)
  {
    MirLocal def = func->stmts[i]->get_def();
    if (def != ~0u)
      defs[def].emplace_back(i, 0);

    std::vector<MirLocal> use = func->stmts[i]->get_uses();
    if (def != ~0u || use.size() > 0 || stmt_info[i].func_call)
      reg_info[i].resize(use.size() + 1, Register::UND);

    if (stmt_info[i].func_call && def == ~0u)
      reg_info[i][0] = Register::X0;

    for (size_t j = 0; j < use.size(); ++j)
    {
      if (use[j] != ~0u)
        uses[use[j]].emplace_back(i, j + 1);
      else
        reg_info[i][j + 1] = Register::X0;
    }
  }

  if (tail_reachable) {
    uses[0].emplace_back(stmt_info.size() - 1, 1);
    reg_info[stmt_info.size() - 1].resize(2, Register::UND);
  } else {
    reg_info[stmt_info.size() - 1].resize(2, Register::UND);
    reg_info[stmt_info.size() - 1][1] = Register::X0;
  }
}

void MirFuncContext::identify_loops(void)
{
  {
    Bitset stmts(stmt_info.size());
    stmts.set_all();

    loops.emplace_back(std::move(stmts),
        std::vector<unsigned int>(), 0,
        std::vector<unsigned int>());
  }

  for (size_t pos = 0; pos < stmt_info.size(); ++pos)
  {
    unsigned int ppos_max = 0;
    for (auto ppos : stmt_info[pos].prev)
      if (ppos > ppos_max)
        ppos_max = ppos;
    if (ppos_max <= pos)
      continue;

    Bitset stmts(stmt_info.size());
    std::queue<unsigned int> queue;
    stmts.set(pos);
    for (auto ppos : stmt_info[pos].prev)
    {
      if (ppos > pos) {
        stmts.set(ppos);
        queue.push(ppos);
      }
    }

    while (!queue.empty())
    {
      unsigned int x = queue.front();
      queue.pop();

      for (auto ppos : stmt_info[x].prev)
      {
        if (stmts.get(ppos))
          continue;
        assert(ppos >= pos && ppos <= ppos_max);
        stmts.set(ppos);
        queue.push(ppos);
      }
    }

    std::vector<unsigned int> tails;
    for (auto stmt : stmts)
      for (auto npos : stmt_info[stmt].next)
        tails.emplace_back(npos);
    tails.erase(
        std::remove_if(
          tails.begin(),
          tails.end(),
          [&] (unsigned int i) {
            if (stmts.get(i))
              return true;
            stmts.set(i);
            return false;
          }),
        tails.end());

    assert(pos > 0);
    stmts.set(pos - 1);

    loops.emplace_back(std::move(stmts),
        std::vector<unsigned int>(), pos - 1, std::move(tails));
  }

  for (size_t i = 1; i < loops.size(); ++i)
  {
    for (size_t j = i - 1; ~j; --j)
    {
      if (loops[j].stmts.contain(loops[i].stmts)) {
        loops[j].kids.emplace_back(i);
        break;
      }
    }
  }
}

void MirFuncContext::prepare(void)
{
  stmt_info.clear();
  loops.clear();

  fill_stmt_info();
  identify_loops();
}

bool MirFuncContext::build_liveness_one(MirLocal local)
{
  if (defs[local].size() == 0)
    return false;
  if (uses[local].size() == 0) {
    assert(local < func->num_args);
    reg_info[0][local + 1] = reg_from_arg_id(local);
    return false;
  }

  const size_t nr_stmts = stmt_info.size();
  const size_t nr_uses = uses[local].size();

  Bitset defined_pos(nr_stmts);
  std::queue<unsigned int> queue;

  assert(local >= func->num_temps || defs[local].size() == 1);
  for (const auto &def : defs[local])
  {
    defined_pos.set(def.first);
    queue.push(def.first);
  }

  while (!queue.empty())
  {
    unsigned int pos = queue.front();
    queue.pop();

    for (auto npos : stmt_info[pos].next)
    {
      if (defined_pos.get(npos))
        continue;
      defined_pos.set(npos);
      queue.push(npos);
    }
  }

  Bitset live_pos(nr_stmts);
  Graph graph(nr_stmts + nr_uses);

  for (size_t i = 0; i < nr_uses; ++i)
  {
    const auto &use = uses[local][i];
    for (auto ppos : stmt_info[use.first].prev)
    {
      if (!defined_pos.get(ppos))
        continue;
      graph.add_edge(nr_stmts + i, ppos);
      if (live_pos.get(ppos))
        continue;
      live_pos.set(ppos);
      queue.push(ppos);
    }
  }

  while (!queue.empty())
  {
    unsigned int pos = queue.front();
    queue.pop();
    if (stmt_info[pos].def == local)
      continue;

    for (auto ppos : stmt_info[pos].prev)
    {
      if (!defined_pos.get(ppos))
        continue;
      graph.add_edge(pos, ppos);
      if (live_pos.get(ppos))
        continue;
      live_pos.set(ppos);
      queue.push(ppos);
    }
  }

  Bitset visited(nr_stmts + nr_uses);

  visited.set(0 + nr_stmts);
  queue.push(0 + nr_stmts);

  Bitset live_stmts(nr_stmts);
  size_t visited_uses = 0, visited_defs = 0;
  uint32_t forbid = 0;
  uint32_t hint = 0;

  while (!queue.empty())
  {
    unsigned int pos = queue.front();
    queue.pop();

    for (auto adj : graph.adjacent(pos))
    {
      if (visited.get(adj))
        continue;
      visited.set(adj);
      queue.push(adj);
    }

    if (pos >= nr_stmts) {
      pos -= nr_stmts;
      const auto &use = uses[local][pos];
      if (stmt_info[use.first].func_call) {
        forbid |= reg_forbid_caller_arg(use.second - 1);
        hint |= reg_hint_caller_arg(use.second - 1);
      }
      if (func->stmts[use.first]->is_return()) {
        forbid |= reg_forbid_return_val();
        hint |= reg_hint_return_val();
      }
      ++visited_uses;
    } else {
      if (pos == 0) {
        assert(local < func->num_args);
        ++visited_defs;
        forbid |= reg_forbid_callee_arg(local);
        hint |= reg_hint_callee_arg(local);
      } else if (stmt_info[pos].def == local) {
        ++visited_defs;
      } else if (stmt_info[pos].func_call) {
        forbid |= reg_forbid_cross_func();
        hint |= reg_hint_cross_func();
      }
      live_stmts.set(pos);
    }
  }
  assert(visited_uses == uses[local].size());
  assert(visited_defs == defs[local].size());

  if (local == 0) {
    forbid |= reg_forbid_return_addr();
    hint |= reg_hint_return_addr();
  }

  MirStmt *remat = nullptr;
  if (defs[local].size() == 1) {
    remat = func->stmts[defs[local][0].first].get();
    if (!remat->can_rematerialize())
      remat = nullptr;
  }

  liveness.emplace_back(
      std::move(live_stmts), local, hint, forbid, remat);

  for (size_t i = 1; i < stmt_info.size(); ++i)
  {
    if (stmt_info[i].def != local)
      continue;
    if (visited.get(i))
      continue;
    reg_info[i][0] = Register::X0;
  }

  return true;
}

void MirFuncContext::build_liveness_all(void)
{
  assert(defs.size() == uses.size());

  std::vector<size_t> indices;
  for (size_t i = 0; i < defs.size(); ++i)
  {
    if (build_liveness_one(i))
      indices.emplace_back(liveness.size() - 1);
    else
      indices.emplace_back(~0ul);
  }

  Graph graph(liveness.size());
  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    std::pair<MirLocal, MirLocal> eq;
    if (!func->stmts[i]->extract_if_assign(eq))
      continue;
    if (eq.second == ~0u)
      continue;
    size_t x = indices[eq.first];
    size_t y = indices[eq.second];
    assert(~x != 0 && ~y != 0);
    if ((liveness[x].hint & MASK_REG_CALLEE)
        != (liveness[y].hint & MASK_REG_CALLEE))
      continue;
    if (liveness[x].stmts.test(liveness[y].stmts))
      continue;
    graph.add_edge(x, y);
  }

  size_t nr_liveness = liveness.size();
  for (size_t i = 0; i < nr_liveness; ++i)
  {
    if (liveness[i].to_spill || graph.adjacent(i).size() == 0)
      continue;
    std::vector<MirLocalLiveness> buffer;
    std::queue<size_t> queue;
    Bitset stmts = liveness[i].stmts;
    uint32_t hint = liveness[i].hint;
    uint32_t forbid = liveness[i].forbid;

    buffer.emplace_back(std::move(liveness[i]));
    queue.push(i);
    liveness[i].to_spill = true;

    while (!queue.empty())
    {
      size_t x = queue.front();
      queue.pop();
      for (auto y : graph.adjacent(x))
      {
        if (liveness[y].to_spill)
          continue;
        if (stmts.test(liveness[y].stmts))
          continue;
        stmts |= liveness[y].stmts;
        hint |= liveness[y].hint;
        forbid |= liveness[y].forbid;

        buffer.emplace_back(std::move(liveness[y]));
        queue.push(y);
        liveness[y].to_spill = true;
      }
    }

    if (buffer.size() == 1)
      liveness[i] = std::move(buffer[0]);
    else
      liveness.emplace_back(
          std::move(stmts), std::move(buffer), hint, forbid);
  }

  liveness.erase(
      std::remove_if(
        liveness.begin(),
        liveness.end(),
        [] (const MirLocalLiveness &ll) { return ll.to_spill; }),
      liveness.end());

  bool to_spill = false;
  for (auto &ll : liveness)
    if (ll.forbid == MASK_REGISTERS)
      to_spill = ll.to_spill = true;

  if (to_spill)
    spill_liveness_all();
}

void MirFuncContext::spill_liveness_one(
    MirLocalLiveness &ll, std::vector<MirLocalLiveness> &buf)
{
  if (ll.kids.size() != 0) {
    for (auto &kid : ll.kids)
      buf.emplace_back(std::move(kid));
    return;
  }

  const auto &loop_indices = loops[ll.loop].kids;

  MirOperands spilled_uses;
  MirOperands spilled_defs;

  std::vector<MirOperands> loop_uses;
  std::vector<MirOperands> loop_defs;
  loop_uses.resize(loop_indices.size());
  loop_defs.resize(loop_indices.size());

  for (const auto &def : ll.get_defs(this))
  {
    for (size_t i = 0; i < loop_indices.size(); ++i)
    {
      unsigned int loop_index = loop_indices[i];
      if (!loops[loop_index].stmts.get(def.first))
        continue;
      loop_defs[i].emplace_back(def);
      goto def_out;
    }
    spilled_defs.emplace_back(def);
  }
def_out:

  for (const auto &use : ll.get_uses(this))
  {
    for (size_t i = 0; i < loop_indices.size(); ++i)
    {
      unsigned int loop_index = loop_indices[i];
      if (!loops[loop_index].stmts.get(use.first))
        continue;
      loop_uses[i].emplace_back(use);
      goto use_out;
    }
    spilled_uses.emplace_back(use);
  }
use_out:

  for (size_t i = 0; i < loop_indices.size(); ++i)
  {
    if (loop_defs[i].size() == 0
        && loop_uses[i].size() == 0)
      continue;
    unsigned int loop_index = loop_indices[i];

    Bitset stmts = loops[loop_index].stmts;
    stmts |= ll.stmts;

    uint32_t hint = 0;
    uint32_t forbid = 0;
    for (const auto &use : loop_uses[i])
    {
      if (stmt_info[use.first].func_call)
      {
        hint |= reg_hint_caller_arg(use.second - 1);
        forbid |= reg_forbid_caller_arg(use.second - 1);
      }
    }
    for (auto stmt : stmts)
      if (stmt_info[stmt].func_call
          && stmt_info[stmt].def != ll.local) {
        forbid |= reg_forbid_cross_func();
        hint |= reg_hint_cross_func();
      }

    buf.emplace_back(std::move(stmts), ll.local,
        loop_index, hint, forbid, std::move(loop_uses[i]),
        std::move(loop_defs[i]), ll.remat);
  }

  for (const auto &use : spilled_uses)
  {
    if (stmt_info[use.first].func_call) {
      Register reg = reg_from_arg_id(use.second);
      spill_loads[stmt_info[use.first].prev[0]]
        .emplace_back(
          std::make_unique<MirSpillLoad>(reg, ll.local));
      reg_info[use.first][use.second] = reg;

      if (auto it = spilled_locals.find(ll.local);
          it == spilled_locals.end())
        spilled_locals[ll.local] = spilled_locals.size();

      continue;
    }

    Bitset stmts(stmt_info.size());
    if (use.first == stmt_info.size() - 1) {
      stmts.set(use.first);
    } else {
      assert(stmt_info[use.first].prev.size() == 1);
      for (auto ppos : stmt_info[use.first].prev)
        stmts.set(ppos);
    }

    uint32_t hint = 0;
    uint32_t forbid = 0;
    if (use.first == stmt_info.size() - 1) {
      assert(ll.local == 0);
      forbid |= reg_forbid_return_addr();
      hint |= reg_hint_return_addr();
    }

    buf.emplace_back(std::move(stmts), ll.local, ~0u,
        hint, forbid, MirOperands { use }, MirOperands(), ll.remat);
  }
  
  for (const auto &def : spilled_defs)
  {
    if (def.first == 0) {
      assert(ll.local == def.second - 1);

      Register reg = reg_from_arg_id(ll.local);
      spill_stores[0].emplace_back(
          std::make_unique<MirSpillStore>(reg, ll.local));
      reg_info[0][ll.local + 1] = reg;

      if (auto it = spilled_locals.find(ll.local);
          it == spilled_locals.end())
        spilled_locals[ll.local] = spilled_locals.size();

      continue;
    }

    Bitset stmts(stmt_info.size());
    stmts.set(def.first);

    uint32_t hint = 0;
    uint32_t forbid = 0;
    buf.emplace_back(std::move(stmts), ll.local, ~0u,
        hint, forbid, MirOperands(), MirOperands { def }, ll.remat);
  }
}

void MirFuncContext::spill_liveness_all(void)
{
  size_t size = liveness.size();
  std::vector<MirLocalLiveness> buffer;

  for (size_t i = 0; i < size; ++i)
  {
    if (liveness[i].to_spill) {
      assert(liveness[i].loop != ~0u);
      spill_liveness_one(liveness[i], buffer);
    } else {
      liveness[i].color = 0;
    }
  }

  liveness.erase(
      std::remove_if(
        liveness.begin(),
        liveness.end(),
        [] (const MirLocalLiveness &ll) { return ll.to_spill; }),
      liveness.end());

  for (auto &ll : buffer)
    liveness.emplace_back(std::move(ll));
}

bool MirFuncContext::graph_try_color(void)
{
  Graph graph(liveness.size());
  std::vector<unsigned int> degree;
  degree.resize(liveness.size(), 0);

  for (size_t i = 0; i < liveness.size(); ++i)
  {
    for (size_t j = i + 1; j < liveness.size(); ++j)
    {
      if (!liveness[i].stmts.test(liveness[j].stmts))
        continue;
      graph.add_edge(i, j);
      ++degree[i];
      ++degree[j];
    }
  }

  for (size_t i = 0; i < liveness.size(); ++i)
    degree[i] += __builtin_popcount(liveness[i].forbid);

  std::stack<unsigned int> stack;
  std::queue<unsigned int> queue;

  for (size_t i = 0; i < liveness.size(); ++i)
    if (degree[i] < NR_REGISTERS)
      queue.push(i);

  while (stack.size() != liveness.size())
  {
    while (!queue.empty())
    {
      unsigned int x = queue.front();
      queue.pop();
      stack.push(x);

      for (auto y : graph.adjacent(x))
        if (degree[y] && --degree[y] == NR_REGISTERS - 1)
          queue.push(y);
      degree[x] = 0;
    }
    if (stack.size() == liveness.size())
      break;

    unsigned int deg_max = 0, node;
    do {
      for (size_t i = 0; i < liveness.size(); ++i)
      {
        if (liveness[i].loop == ~0u)
          continue;
        if (degree[i] <= deg_max)
          continue;
        if (!liveness[i].remat)
          continue;
        deg_max = degree[i];
        node = i;
      }
      if (deg_max >= NR_REGISTERS)
        break;

      for (size_t i = 0; i < liveness.size(); ++i)
      {
        if (liveness[i].loop == ~0u)
          continue;
        if (degree[i] <= deg_max)
          continue;
        if (liveness[i].kids.size() == 0)
          continue;
        deg_max = degree[i];
        node = i;
      }
      if (deg_max >= NR_REGISTERS)
        break;

      for (size_t i = 0; i < liveness.size(); ++i)
      {
        if (liveness[i].loop == ~0u)
          continue;
        if (degree[i] <= deg_max)
          continue;
        deg_max = degree[i];
        node = i;
      }
      assert(deg_max >= NR_REGISTERS);
    } while (0);

    stack.push(node);
    for (auto y : graph.adjacent(node))
      if (degree[y] && --degree[y] == NR_REGISTERS - 1)
        queue.push(y);
    degree[node] = 0;
  }

  bool need_spill = false;
  while (!stack.empty())
  {
    unsigned int x = stack.top();
    stack.pop();
    assert(liveness[x].color == 0);
    assert(!liveness[x].to_spill);

    uint32_t color = liveness[x].forbid;
    for (auto y : graph.adjacent(x))
      color |= liveness[y].color;

    if (color != MASK_REGISTERS) {
      unsigned int c;
      do {
        c = ~color & (liveness[x].hint & MASK_REG_CALLEE);
        if (c)
          break;
        c = ~color & liveness[x].hint;
        if (c)
          break;
        c = ~color;
      } while (0);
      c = __builtin_ctz(c);
      liveness[x].color = 1u << c;
      continue;
    }

    liveness[x].to_spill = true;
    need_spill = true;
  }

  return !need_spill;
}

void MirFuncContext::finish_liveness(const MirLocalLiveness &ll, uint32_t color)
{
  assert(color != 0 && !(color & ll.forbid));
  if (ll.kids.size() != 0) {
    for (auto &kid : ll.kids)
      finish_liveness(kid, color);
    return;
  }

  unsigned int regid = __builtin_ctz(color);
  Register reg = static_cast<Register>(regid);

  if (regid >= NR_REG_CALLER
      && regid - NR_REG_CALLER + 1 > num_callee_regs)
    num_callee_regs = regid - NR_REG_CALLER + 1;

  for (const auto &def : ll.get_defs(this))
  {
    auto &regs = reg_info[def.first];
    if (ll.remat && !ll.get_uses(this).size())
      regs[def.second] = Register::X0;
    else
      regs[def.second] = reg;
  }

  for (const auto &use : ll.get_uses(this))
  {
    auto &regs = reg_info[use.first];
    regs[use.second] = reg;
  }

  if (ll.loop == 0)
    return;
  if (~ll.loop && (ll.color & MASK_REG_CALLER))
    return;

  if (!ll.remat) {
    auto it = spilled_locals.find(ll.local);
    if (it == spilled_locals.end())
      spilled_locals[ll.local] = spilled_locals.size();
  }

  if (!ll.remat && ~ll.loop && ll.get_defs(this).size() != 0) {
    const auto &loop = loops[ll.loop];
    for (auto tail : loop.tails)
    {
      if (!ll.stmts.get(tail))
        continue;
      spill_stores[tail].emplace_back(
          std::make_unique<MirSpillStore>(reg, ll.local));
    }
  } else if (!ll.remat && ll.get_defs(this).size() != 0) {
    for (auto stmt : ll.stmts)
    {
      spill_stores[stmt].emplace_back(
          std::make_unique<MirSpillStore>(reg, ll.local));
      break;
    }
  }

  if (~ll.loop && ll.get_uses(this).size() != 0) {
    const auto &loop = loops[ll.loop];
    if (ll.stmts.get(loop.head)) {
      if (!ll.remat) {
        spill_loads[loop.head].emplace_back(
            std::make_unique<MirSpillLoad>(reg, ll.local));
      } else {
        spill_loads[loop.head].emplace_back(ll.remat->rematerialize(reg));
      }
    }
  } else if (ll.get_uses(this).size() != 0) {
    for (auto stmt : ll.stmts)
    {
      if (!ll.remat) {
        spill_loads[stmt].emplace_back(
            std::make_unique<MirSpillLoad>(reg, ll.local));
      } else {
        spill_loads[stmt].emplace_back(ll.remat->rematerialize(reg));
      }
      break;
    }
  }
}

void MirFuncContext::spill_regs_cross_func(void)
{
  std::vector<Bitset> defs_in_loops;
  std::vector<Bitset> uses_in_loops;

  defs_in_loops.resize(loops.size(), Bitset(num_phis));
  uses_in_loops.resize(loops.size(), Bitset(num_phis));

  std::vector<size_t> stmt_to_loop;
  stmt_to_loop.resize(stmt_info.size(), 0);

  for (size_t i = 1; i < loops.size(); ++i)
    for (auto stmt : loops[i].stmts)
      stmt_to_loop[stmt] = i;

  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    if (stmt_info[i].def != ~0u)
      defs_in_loops[stmt_to_loop[i]].set(stmt_info[i].def);
    for (auto use : func->stmts[i]->get_uses())
      if (use != ~0u)
        uses_in_loops[stmt_to_loop[i]].set(use);
  }

  for (size_t i = loops.size() - 1; ~i; --i)
  {
    for (auto kid : loops[i].kids)
    {
      defs_in_loops[i] |= defs_in_loops[kid];
      uses_in_loops[i] |= uses_in_loops[kid];
    }
  }

  for (size_t i = 0; i < liveness.size(); ++i)
  {
    if (liveness[i].color & MASK_REG_CALLEE)
      continue;
    if (liveness[i].loop == ~0u)
      continue;

    Bitset loads(stmt_info.size());
    Bitset stores(stmt_info.size());

    Bitset mask(stmt_info.size());
    std::queue<size_t> queue;

    for (auto stmt : liveness[i].stmts)
    {
      if (!stmt_info[stmt].func_call)
        continue;
      loads.set(stmt);
      if (!liveness[i].remat)
        stores.set(stmt_info[stmt].prev[0]);
    }

    if (unsigned int loop = liveness[i].loop;
        loop != ~0u && loop != 0) {
      if (liveness[i].stmts.get(loops[loop].head))
        loads.set(loops[loop].head);
      if (!liveness[i].remat)
        for (auto tail : loops[loop].tails)
          if (liveness[i].stmts.get(tail))
            stores.set(tail);
    }

    std::vector<MirLocal> locals;
    if (liveness[i].kids.size() == 0) {
      locals.emplace_back(liveness[i].local);
    } else {
      for (const auto &kid : liveness[i].kids)
        locals.emplace_back(kid.local);
    }

    if (!liveness[i].remat) {
      for (auto local : locals)
      {
        for (auto def : defs[local])
        {
          if (!liveness[i].stmts.get(def.first))
            continue;
          mask.set(def.first);
          queue.push(def.first);
        }
      }
  
      while (!queue.empty())
      {
        size_t stmt = queue.front();
        queue.pop();
  
        for (size_t npos : stmt_info[stmt].next)
        {
          if (mask.get(npos))
            continue;
          if (!liveness[i].stmts.get(npos))
            continue;
          if (stmt_info[npos].func_call)
            continue;
          mask.set(npos);
          queue.push(npos);
        }
      }
  
      stores &= mask;
      mask.reset();
    }

    for (auto local : locals)
      for (auto use : uses[local])
        queue.push(use.first);

    for (auto stmt : stores)
      queue.push(stmt);

    while (!queue.empty())
    {
      size_t stmt = queue.front();
      queue.pop();

      for (auto ppos : stmt_info[stmt].prev)
      {
        if (mask.get(ppos))
          continue;
        if (!liveness[i].stmts.get(ppos))
          continue;
        if (std::find(locals.begin(), locals.end(),
              stmt_info[ppos].def) != locals.end())
          continue;
        mask.set(ppos);
        if (stmt_info[ppos].func_call)
          continue;
        queue.push(ppos);
      }
    }

    loads &= mask;

    for (size_t loop = ~liveness[i].loop ? liveness[i].loop + 1 : 0;
        loop < loops.size();
        ++loop)
    {
      if (!liveness[i].remat) {
        bool has_def = false;
        for (auto local : locals)
          has_def |= defs_in_loops[loop].get(local);
        if (!has_def && stores.test(loops[loop].stmts)) {
          assert(liveness[i].stmts.get(loops[loop].head));
          stores -= loops[loop].stmts;
          stores.set(loops[loop].head);
        }
      }

      bool has_use = false;
      for (auto local : locals)
        has_use |= uses_in_loops[loop].get(local);
      if (!has_use && loads.test(loops[loop].stmts)) {
        loads -= loops[loop].stmts;
        bool ok = false;
        for (auto tail : loops[loop].tails)
        {
          if (liveness[i].stmts.get(tail)) {
            ok = true;
            loads.set(tail);
          }
        }
        assert(ok);
      }
    }

    if (!liveness[i].remat && (stores || loads)) {
      if (auto it = spilled_locals.find(locals[0]);
          it == spilled_locals.end())
        spilled_locals[locals[0]] = spilled_locals.size();
    }

    unsigned int regid = __builtin_ctz(liveness[i].color);
    Register reg = static_cast<Register>(regid);

    if (!liveness[i].remat) {
      for (auto stmt : stores)
      {
        assert(stmt > 0);
        spill_stores[stmt].emplace_back(
            std::make_unique<MirSpillStore>(reg, locals[0]));
      }
    }

    for (auto stmt : loads)
    {
      spill_loads[stmt].emplace_back(liveness[i].remat
          ? liveness[i].remat->rematerialize(reg)
          : std::make_unique<MirSpillLoad>(reg, locals[0]));
    }
  }
}

void MirFuncContext::finish_reg_alloc(void)
{
  for (const auto &ll : liveness)
    finish_liveness(ll, ll.color);

  spill_regs_cross_func();
}

void MirFuncContext::reg_alloc(void)
{
  fill_defs_and_uses();
  build_liveness_all();

  while (!graph_try_color())
    spill_liveness_all();

  finish_reg_alloc();
}
