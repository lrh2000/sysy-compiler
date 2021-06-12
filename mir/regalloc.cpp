#include <queue>
#include <stack>
#include "mir.h"
#include "context.h"
#include "../asm/register.h"
#include "../utils/bitset.h"
#include "../utils/graph.h"

const MirFuncContext::SpillOps MirFuncContext::g_no_spill_ops = {};

struct MirStmtInfo
{
  MirStmtInfo(std::vector<unsigned int> &&next,
      std::vector<unsigned int> &&prev,
      MirLocal def, bool func_call)
    : next(std::move(next)), prev(std::move(prev)),
      def(def), func_call(func_call)
  {}

  std::vector<unsigned int> next;
  std::vector<unsigned int> prev;
  MirLocal def;
  bool func_call;
};

struct MirLoop
{
  MirLoop(Bitset &&stmts, std::vector<unsigned int> &&kids,
      unsigned int head, std::vector<unsigned int> &&tails)
    : stmts(std::move(stmts)), kids(std::move(kids)),
      head(head), tails(std::move(tails))
  {}

  Bitset stmts;
  std::vector<unsigned int> kids;
  unsigned int head;
  std::vector<unsigned int> tails;
};

struct MirLocalLiveness
{
  MirLocalLiveness(Bitset &&stmts,
      MirOperands &&defs, MirOperands &&uses,
      MirLocal local, unsigned int loop, uint32_t color_mask)
    : stmts(std::move(stmts)),
      defs(std::move(defs)), uses(std::move(uses)),
      local(local), loop(loop), color_mask(color_mask),
      to_spill(false), color(0)
  {}

  Bitset stmts;
  MirOperands defs;
  MirOperands uses;
  MirLocal local;
  unsigned int loop;
  uint32_t color_mask;
  bool to_spill;
  uint32_t color;
};

MirFuncContext::MirFuncContext(
    MirFuncItem *func, AsmBuilder *builder)
  : func(func), stmt_info(), defs(), uses(),
    liveness(), loops(), reg_info(), spilled_locals(),
    spill_loads(), spill_stores(), num_callee_regs(0),
    builder(builder)
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
  defs.resize(func->num_temps);
  uses.resize(func->num_temps);

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
    if (def != ~0u || use.size() > 0)
      reg_info[i].resize(use.size() + 1, Register::UND);

    for (size_t j = 0; j < use.size(); ++j)
    {
      if (use[j] != ~0u)
        uses[use[j]].emplace_back(i, j + 1);
      else
        reg_info[i][j + 1] = Register::X0;
    }
  }

  uses[0].emplace_back(stmt_info.size() - 1, 1);
  reg_info[stmt_info.size() - 1].resize(2, Register::UND);
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
    queue.push(pos);

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
    for (auto tail : tails)
      stmts.set(tail);
    assert(pos > 0);
    stmts.set(pos - 1);

    loops.emplace_back(std::move(stmts),
        std::vector<unsigned int>(), pos - 1, std::move(tails));
  }

  std::stack<size_t> stack;

  for (size_t i = 0; i < loops.size(); ++i)
  {
    while (!stack.empty()
        && !loops[stack.top()].stmts.contains(loops[i].stmts))
      stack.pop();
    if (!stack.empty())
      loops[stack.top()].kids.emplace_back(i);
    stack.push(i);
  }
}

void MirFuncContext::prepare(void)
{
  fill_stmt_info();
  fill_defs_and_uses();
  identify_loops();
}

void MirFuncContext::build_liveness_one(MirLocal local)
{
  const size_t nr_stmts = stmt_info.size();
  const size_t nr_uses = uses[local].size();

  Bitset defined_pos(nr_stmts);
  std::queue<unsigned int> queue;

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

  for (size_t i = 0; i < nr_uses; ++i)
  {
    if (visited.get(i + nr_stmts))
      continue;
    visited.set(i + nr_stmts);
    queue.push(i + nr_stmts);

    Bitset live_stmts(nr_stmts);
    MirOperands defs;
    MirOperands uses;
    uint32_t color_mask = 0;

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
        const auto &use = this->uses[local][pos];
        uses.emplace_back(use);
        if (stmt_info[use.first].func_call)
          color_mask |= (1u << use.second) - 2;
      } else {
        if (pos == 0) {
          assert(local < func->num_args);
          defs.emplace_back(pos, (unsigned int) local + 1);
          color_mask |= (1u << local) - 1;
        } else if (stmt_info[pos].def == local) {
          defs.emplace_back(pos, 0u);
        } else if (stmt_info[pos].func_call) {
          color_mask |= MASK_REG_CALLER;
        }
        live_stmts.set(pos);
      }
    }

    if (local == 0) {
      color_mask |= MASK_REG_CALLEE;
      color_mask |= 1u << static_cast<uint32_t>(Register::A0);
    }

    liveness.emplace_back(std::move(live_stmts),
        std::move(defs), std::move(uses), local, 0, color_mask);
  }

  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    if (stmt_info[i].def != local)
      continue;
    if (visited.get(i))
      continue;
    reg_info[i][0] = Register::X0;
  }
}

void MirFuncContext::build_liveness_all(void)
{
  assert(defs.size() == uses.size());

  for (size_t i = 0; i < defs.size(); ++i)
    build_liveness_one(i);

  bool to_spill = false;
  for (auto &ll : liveness)
    if (ll.color_mask == MASK_REGISTERS)
      to_spill = ll.to_spill = true;

  if (to_spill)
    spill_liveness_all();
}

void MirFuncContext::spill_liveness_one(const MirLocalLiveness &ll)
{
  const auto &loop_indices = loops[ll.loop].kids;

  MirOperands spilled_uses;
  MirOperands spilled_defs;

  std::vector<MirOperands> loop_uses;
  std::vector<MirOperands> loop_defs;
  loop_uses.resize(loop_indices.size());
  loop_defs.resize(loop_indices.size());

  for (const auto &def : ll.defs)
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

  for (const auto &use : ll.uses)
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

    uint32_t color_mask = 0;
    for (const auto &use : loop_uses[i])
      if (stmt_info[use.first].func_call)
        color_mask |= (1u << use.second) - 2;
    for (auto stmt : stmts)
      if (stmt_info[stmt].func_call
          && stmt_info[stmt].def != ll.local)
        color_mask |= MASK_REG_CALLER;

    liveness.emplace_back(std::move(stmts),
        std::move(loop_defs[i]), std::move(loop_uses[i]),
        ll.local, loop_index, color_mask);
  }

  for (const auto &use : spilled_uses)
  {
    Bitset stmts(stmt_info.size());
    stmts.set(use.first);

    uint32_t color_mask = 0;
    if (stmt_info[use.first].func_call)
      color_mask |= (1u << use.second) - 2;
    if (use.first == stmt_info.size() - 1) {
      assert(ll.local == 0);
      color_mask |= MASK_REG_CALLEE;
      color_mask |= 1u << static_cast<uint32_t>(Register::A0);
    }

    liveness.emplace_back(
        std::move(stmts), MirOperands(),
        MirOperands { use }, ll.local, ~0u, color_mask);
  }
  
  for (const auto &def : spilled_defs)
  {
    if (def.first == 0) {
      assert(ll.local == def.second - 1);

      Register reg = static_cast<Register>(ll.local);
      spill_stores[0].emplace_back(ll.local, reg);
      reg_info[0][ll.local + 1] = reg;

      if (auto it = spilled_locals.find(ll.local);
          it == spilled_locals.end())
        spilled_locals[ll.local] = spilled_locals.size();

      continue;
    }

    Bitset stmts(stmt_info.size());
    stmts.set(def.first);

    liveness.emplace_back(
        std::move(stmts), MirOperands { def },
        MirOperands(), ll.local, ~0u, 0);
  }
}

void MirFuncContext::spill_liveness_all(void)
{
  size_t size = liveness.size();

  bool can_spill = false;
  for (size_t i = 0; i < size; ++i)
    if (liveness[i].to_spill && liveness[i].loop != ~0u)
      can_spill = true;
    else
      liveness[i].to_spill = false;
  assert(can_spill);

  for (size_t i = 0; i < size; ++i)
    if (liveness[i].to_spill)
      spill_liveness_one(liveness[i]);

  liveness.erase(
      std::remove_if(
        liveness.begin(),
        liveness.end(),
        [] (const MirLocalLiveness &ll) { return ll.to_spill; }),
      liveness.end());
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
    degree[i] += __builtin_popcount(liveness[i].color_mask);

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
        if (--degree[y] == NR_REGISTERS - 1)
          queue.push(y);
      degree[x] = 0;
    }
    if (stack.size() == liveness.size())
      break;

    unsigned int deg_max = 0, node;
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

    stack.push(node);
    for (auto y : graph.adjacent(node))
      if (--degree[y] == NR_REGISTERS - 1)
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

    uint32_t color = liveness[x].color_mask;
    for (auto y : graph.adjacent(x))
      color |= liveness[y].color;
    if (color != MASK_REGISTERS) {
      unsigned int c = __builtin_ctz(~color);
      liveness[x].color = 1u << c;
      continue;
    }

    liveness[x].to_spill = true;
    need_spill = true;
  }

  return !need_spill;
}

void MirFuncContext::finish_liveness(const MirLocalLiveness &ll)
{
  assert(ll.color != 0);
  unsigned int regid = __builtin_ctz(ll.color);
  Register reg = static_cast<Register>(regid);

  if (regid >= NR_REG_CALLER
      && regid - NR_REG_CALLER + 1 > num_callee_regs)
    num_callee_regs = regid - NR_REG_CALLER + 1;

  for (const auto &def : ll.defs)
  {
    auto &regs = reg_info[def.first];
    regs[def.second] = reg;
  }

  for (const auto &use : ll.uses)
  {
    auto &regs = reg_info[use.first];
    regs[use.second] = reg;
  }

  if (ll.loop == 0)
    return;

  if (auto it = spilled_locals.find(ll.local);
      it == spilled_locals.end())
    spilled_locals[ll.local] = spilled_locals.size();

  if (~ll.loop && ll.defs.size() != 0) {
    const auto &loop = loops[ll.loop];
    for (auto tail : loop.tails)
    {
      if (!ll.stmts.get(tail))
        continue;
      spill_stores[tail].emplace_back(ll.local, reg);
    }
  } else if (ll.defs.size() != 0) {
    for (auto stmt : ll.stmts)
    {
      spill_stores[stmt].emplace_back(ll.local, reg);
      break;
    }
  }

  if (~ll.loop && ll.uses.size() != 0) {
    const auto &loop = loops[ll.loop];
    if (ll.stmts.get(loop.head))
      spill_loads[loop.head].emplace_back(ll.local, reg);
  } else if (ll.uses.size() != 0) {
    for (auto stmt : ll.stmts)
    {
      spill_loads[stmt].emplace_back(ll.local, reg);
      break;
    }
  }
}

void MirFuncContext::finish_reg_alloc(void)
{
  for (const auto &ll : liveness)
    finish_liveness(ll);
}

void MirFuncContext::reg_alloc(void)
{
  build_liveness_all();

  while (!graph_try_color())
    spill_liveness_all();

  finish_reg_alloc();
}
