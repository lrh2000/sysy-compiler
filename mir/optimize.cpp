#include <utility>
#include <queue>
#include <stack>
#include "mir.h"
#include "context.h"
#include "context_impl.h"
#include "../utils/bitset.h"
#include "../utils/hash.h"

typedef std::pair<const MirStmt *, unsigned int> MirCachedStmt;

struct MirStmtEqual
{
  bool operator ()(
      const MirCachedStmt &lhs, const MirCachedStmt &rhs) const
  {
    return lhs.second == rhs.second && lhs.first->equal(rhs.first);
  }
};

struct MirStmtHash
{
  size_t operator ()(const MirCachedStmt &stmt) const
  {
    size_t result = stmt.first->hash();
    hash_combine(result, stmt.second);
    return result;
  }
};

#define replace_if_possible(variable)       \
  do {                                      \
    if (auto it = rules.find(variable);     \
        it != rules.end())                  \
      variable = it->second;                \
  } while (0);

bool MirEmptyStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  return false;
}

size_t MirStmt::hash(void) const
{
  abort();
}

bool MirStmt::equal(const MirStmt *other) const
{
  abort();
}

bool MirSymbolAddrStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  return true;
}

size_t MirSymbolAddrStmt::hash(void) const
{
  size_t result = 0;
  hash_combine(result, name);
  hash_combine(result, offset);
  return result;
}

bool MirSymbolAddrStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirSymbolAddrStmt *>(other);
  if (!stmt)
    return false;
  return name == stmt->name && offset == stmt->offset;
}

bool MirArrayAddrStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  return true;
}

size_t MirArrayAddrStmt::hash(void) const
{
  size_t result = 1;
  hash_combine(result, id);
  hash_combine(result, offset);
  return result;
}

bool MirArrayAddrStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirArrayAddrStmt *>(other);
  if (!stmt)
    return false;
  return id == stmt->id && offset == stmt->offset;
}

bool MirImmStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  return true;
}

size_t MirImmStmt::hash(void) const
{
  size_t result = 2;
  hash_combine(result, value);
  return result;
}

bool MirImmStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirImmStmt *>(other);
  if (!stmt)
    return false;
  return value == stmt->value;
}

bool MirBinaryStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(src1);
  replace_if_possible(src2);
  return true;
}

size_t MirBinaryStmt::hash(void) const
{
  size_t result = 3;
  hash_combine(result, src1);
  hash_combine(result, src2);
  hash_combine(result, static_cast<size_t>(op));
  return result;
}

bool MirBinaryStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirBinaryStmt *>(other);
  if (!stmt)
    return false;
  return src1 == stmt->src1
    && src2 == stmt->src2 && op == stmt->op;
}

bool MirBinaryImmStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(src1);
  return true;
}

size_t MirBinaryImmStmt::hash(void) const
{
  size_t result = 4;
  hash_combine(result, src1);
  hash_combine(result, src2);
  hash_combine(result, static_cast<size_t>(op));
  return result;
}

bool MirBinaryImmStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirBinaryImmStmt *>(other);
  if (!stmt)
    return false;
  return src1 == stmt->src1
    && src2 == stmt->src2 && op == stmt->op;
}

bool MirUnaryStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(src);
  return true;
}

size_t MirUnaryStmt::hash(void) const
{
  size_t result = 5;
  hash_combine(result, src);
  hash_combine(result, static_cast<size_t>(op));
  return result;
}

bool MirUnaryStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirUnaryStmt *>(other);
  if (!stmt)
    return false;
  return src == stmt->src && op == stmt->op;
}

bool MirLoadStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(address);
  return true;
}

size_t MirLoadStmt::hash(void) const
{
  size_t result = 6;
  hash_combine(result, address);
  hash_combine(result, offset);
  return result;
}

bool MirLoadStmt::equal(const MirStmt *other) const
{
  auto stmt = dynamic_cast<const MirLoadStmt *>(other);
  if (!stmt)
    return false;
  return address == stmt->address && offset == stmt->offset;
}

void MirEmptyStmt::replace(MirLocal local, MirLocal new_local)
{ /* nothing */ }

void MirSymbolAddrStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
}

void MirArrayAddrStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
}

void MirImmStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
}

void MirBinaryStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
  if (src1 == local)
    src1 = new_local;
  if (src2 == local)
    src2 = new_local;
}

void MirBinaryImmStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
  if (src1 == local)
    src1 = new_local;
}

void MirUnaryStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
  if (src == local)
    src = new_local;
}

void MirCallStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(local != dest);
  for (auto &arg : args)
    if (arg == local)
      arg = new_local;
}

bool MirCallStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  for (auto &arg : args)
    replace_if_possible(arg);
  return false;
}

void MirBranchStmt::replace(MirLocal local, MirLocal new_local)
{
  if (src1 == local)
    src1 = new_local;
  if (src2 == local)
    src2 = new_local;
}

bool MirBranchStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(src1);
  replace_if_possible(src2);
  return false;
}

void MirJumpStmt::replace(MirLocal local, MirLocal new_local)
{ /* nothing */ }

bool MirJumpStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  return false;
}

void MirStoreStmt::replace(MirLocal local, MirLocal new_local)
{
  if (address == local)
    address = new_local;
  if (value == local)
    value = new_local;
}

bool MirStoreStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(address);
  replace_if_possible(value);
  return false;
}

void MirLoadStmt::replace(MirLocal local, MirLocal new_local)
{
  assert(dest != local);
  if (address == local)
    address = new_local;
}

void MirReturnStmt::replace(MirLocal local, MirLocal new_local)
{
  if (has_value && value == local)
    value = new_local;
}

bool MirReturnStmt::apply_rules(
    const std::unordered_map<MirLocal, MirLocal> &rules)
{
  replace_if_possible(value);
  return false;
}

void MirStmt::remove_dest(void)
{
  abort();
}

void MirCallStmt::remove_dest(void)
{
  dest = ~0u;
}

Bitset MirFuncContext::identify_invariants(const MirLoop &loop)
{
  struct Node
  {
    Node(unsigned int in_degree, unsigned int stmt_id)
      : in_degree(in_degree), stmt_id(stmt_id), out_edges()
    {}

    unsigned int in_degree;
    unsigned int stmt_id;
    std::vector<MirLocal> out_edges;
  };

  std::unordered_map<MirLocal, Node> depinfo;
  bool has_mem_store = false;

  for (auto stmt : loop.stmts)
    has_mem_store |= func->stmts[stmt]->maybe_mem_store();

  for (auto stmt : loop.stmts)
  {
    if (stmt_info[stmt].def == ~0u)
      continue;
    unsigned int in_degree = 0;
    if (stmt_info[stmt].def < func->num_locals)
      in_degree = 1;
    if (stmt_info[stmt].func_call)
      in_degree = 1;
    if (has_mem_store && func->stmts[stmt]->is_mem_load())
      in_degree = 1;
    depinfo.insert(std::make_pair(
          stmt_info[stmt].def, Node(in_degree, stmt)));
  }

  for (auto &dep : depinfo)
  {
    std::vector<MirLocal> uses =
      func->stmts[dep.second.stmt_id]->get_uses();
    for (auto use : uses)
    {
      auto it = depinfo.find(use);
      if (it == depinfo.end())
        continue;
      it->second.out_edges.emplace_back(dep.first);
      ++dep.second.in_degree;
    }
  }

  std::queue<MirLocal> queue;
  for (const auto &dep : depinfo)
    if (dep.second.in_degree == 0)
      queue.push(dep.first);

  Bitset result(stmt_info.size());

  while (!queue.empty())
  {
    MirLocal local = queue.front();
    queue.pop();

    auto it = depinfo.find(local);
    assert(it != depinfo.end());
    assert(it->second.in_degree == 0);

    result.set(it->second.stmt_id);
    for (auto next : it->second.out_edges)
    {
      auto itn = depinfo.find(next);
      assert(itn != depinfo.end());
      assert(itn->second.in_degree > 0);

      if (--itn->second.in_degree == 0)
        queue.push(next);
    }
  }

  return result;
}

void MirFuncContext::move_invariants(void)
{
  if (loops.size() == 1)
    return;

  std::vector<Bitset> invariants;
  std::unordered_map<size_t, size_t> stmt_to_loop;

  for (size_t i = 1; i < loops.size(); ++i)
  {
    invariants.emplace_back(
        identify_invariants(loops[i]));
    stmt_to_loop[loops[i].head] = i;
  }

  Bitset all_invariants = invariants[0];
  for (size_t i = 1; i < invariants.size(); ++i)
  {
    invariants[i] -= all_invariants;
    all_invariants |= invariants[i];
  }

  std::vector<std::unique_ptr<MirStmt>> stmts;
  std::vector<size_t> labels;
  labels.resize(func->labels.size());

  std::vector<std::pair<size_t, MirLabel>> sorted_labels;
  for (unsigned int i = 0; i < func->labels.size(); ++i)
    sorted_labels.emplace_back(func->labels[i], i);
  std::sort(sorted_labels.begin(), sorted_labels.end());

  size_t j = 0;
  for (size_t i = 0; i < func->stmts.size(); ++i)
  {
    bool has_label =
      j < sorted_labels.size() && sorted_labels[j].first == i;
    bool put_stmt = !all_invariants.get(i);
    bool begin_loop = stmt_to_loop.find(i) != stmt_to_loop.end();

    assert(!has_label || put_stmt);
    assert(!begin_loop || put_stmt);
    assert(!begin_loop || !has_label);

    if (begin_loop) {
      for (auto stmt : invariants[stmt_to_loop[i] - 1])
        stmts.emplace_back(std::move(func->stmts[stmt]));
      stmts.emplace_back(std::move(func->stmts[i]));
    } else {
      if (has_label) {
        labels[sorted_labels[j].second] = stmts.size();
        ++j;
      }
      if (put_stmt)
        stmts.emplace_back(std::move(func->stmts[i]));
    }
  }
  assert(j == sorted_labels.size());

  func->stmts = std::move(stmts);
  func->labels = std::move(labels);

  prepare();
}

void MirFuncContext::convert_one_to_ssa(
    MirLocal local, MirFuncContext::PhiPosAndOps &phi_ops)
{
  std::vector<unsigned int> degrees;
  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    unsigned int degree = 0;
    for (auto ppos : stmt_info[i].prev)
      if (ppos < i)
        ++degree;
    degrees.emplace_back(degree);
  }

  std::vector<unsigned int> stmt_version;
  stmt_version.resize(stmt_info.size(), ~0);

  std::queue<unsigned int> queue;
  for (size_t i = stmt_info.size() - 1; ~i; --i)
  {
    if (degrees[i] != 0)
      continue;
    stmt_version[i] = 0;
    for (auto npos : stmt_info[i].next)
      if (npos > i && --degrees[npos] == 0)
        queue.push(npos);
  }

  std::vector<MirLocal> version_local;
  version_local.emplace_back(~0u);

  if (local < func->num_args) {
    stmt_version[0] = 1;
    version_local.emplace_back(local);
  }

  std::unordered_map<size_t, size_t> stmt_to_loop;
  for (size_t i = 1; i < loops.size(); ++i)
    stmt_to_loop[loops[i].head] = i;

  while (!queue.empty())
  {
    unsigned int pos = queue.front();
    queue.pop();

    if (auto it = stmt_to_loop.find(pos);
        it != stmt_to_loop.end()) {
      const auto &loop = loops[it->second];
      bool found = false;
      for (auto stmt : loop.stmts)
      {
        if (stmt_info[stmt].def != local)
          continue;
        found = true;
        break;
      }

      unsigned int oldv;
      assert(stmt_info[pos].prev.size() == 1);
      for (auto ppos : stmt_info[pos].prev)
        oldv = stmt_version[ppos];
      assert(oldv != ~0u);

      if (!found) {
        stmt_version[++pos] = oldv;
      } else {
        MirLocal phi = new_phi();
        version_local.emplace_back(phi);
        phi_ops[pos].emplace_back(phi, version_local[oldv]);
        stmt_version[++pos] = version_local.size() - 1;
      }
    } else if (stmt_info[pos].def == local) {
      std::pair<MirLocal, MirLocal> eq;
      bool ok = func->stmts[pos]->extract_if_assign(eq);
      assert(ok && eq.second >= func->num_locals);
      version_local.emplace_back(eq.second);
      stmt_version[pos] = version_local.size() - 1;
    } else {
      unsigned int oldv, cnt = 0;
      for (auto ppos : stmt_info[pos].prev)
      {
        unsigned int ver = stmt_version[ppos];
        assert(ver != ~0u);
        if (cnt == 1 && (ver == 0 || oldv == ver)) {
          continue;
        } else if (cnt == 1 && oldv != 0) {
          cnt = 2;
          break;
        }
        oldv = ver;
        cnt = 1;
      }
      if (cnt > 1) {
        MirLocal phi = new_phi();
        version_local.emplace_back(phi);
        for (auto ppos : stmt_info[pos].prev)
        {
          unsigned int ver = stmt_version[ppos];
          assert(ver != ~0u);
          phi_ops[ppos].emplace_back(phi, version_local[ver]);
        }
        stmt_version[pos] = version_local.size() - 1;
      } else {
        assert(cnt == 1);
        stmt_version[pos] = oldv;
        func->stmts[pos]->replace(local, version_local[oldv]);
      }
    }

    for (auto npos : stmt_info[pos].next)
    {
      if (npos < pos) {
        unsigned int newv = stmt_version[npos];
        unsigned int ver = stmt_version[pos];
        assert(newv != ~0u && ver != ~0u);
        if (newv != ver) {
          phi_ops[pos].emplace_back(
              version_local[newv], version_local[ver]);
        }
        continue;
      }

      assert(degrees[npos] > 0);
      if (--degrees[npos] == 0)
        queue.push(npos);
    }
  }

  if (local == 0) {
    switch (stmt_version[stmt_info.size() - 1])
    {
    case 0:
      tail_reachable = false;
      break;
    case 1:
      tail_reachable = true;
      break;
    default:
      abort();
    }
  }
}

void MirFuncContext::convert_all_to_ssa(void)
{
  PhiPosAndOps phi_ops;

  for (unsigned int i = 0; i < func->num_locals; ++i)
    convert_one_to_ssa(i, phi_ops);

  std::vector<std::unique_ptr<MirStmt>> stmts;
  std::vector<size_t> labels;
  labels.resize(func->labels.size());

  std::vector<std::pair<size_t, MirLabel>> sorted_labels;
  for (unsigned int i = 0; i < func->labels.size(); ++i)
    sorted_labels.emplace_back(func->labels[i], i);
  std::sort(sorted_labels.begin(), sorted_labels.end());

  size_t j = 0;
  for (size_t i = 0; i < func->stmts.size(); ++i)
  {
    bool is_label
      = j < sorted_labels.size() && sorted_labels[j].first == i;
    bool is_branch = func->stmts[i]->is_jump_or_branch();
    bool has_phi = phi_ops.find(i) != phi_ops.end();
    bool keep = stmt_info[i].def >= func->num_locals;

    if (is_label) {
      assert(!is_branch && keep);
      labels[sorted_labels[j++].second] = stmts.size();
      stmts.emplace_back(std::move(func->stmts[i]));
      keep = false;
    }

    if (is_branch && has_phi) {
      for (const auto &op : phi_ops[i])
      {
        stmts.emplace_back(
            std::make_unique<MirUnaryStmt>(
              op.first, op.second, MirUnaryOp::Nop));
      }
      has_phi = false;
    }

    if (keep) {
      stmts.emplace_back(std::move(func->stmts[i]));
    }

    if (has_phi) {
      for (const auto &op : phi_ops[i])
      {
        stmts.emplace_back(
            std::make_unique<MirUnaryStmt>(
              op.first, op.second, MirUnaryOp::Nop));
      }
    }
  }
  assert(j == sorted_labels.size());

  func->stmts = std::move(stmts);
  func->labels = std::move(labels);

  prepare();
}

void MirFuncContext::merge_duplicates(void)
{
  std::vector<unsigned int> degrees;
  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    unsigned int degree = 0;
    for (auto ppos : stmt_info[i].prev)
      if (ppos < i)
        ++degree;
    degrees.emplace_back(degree);
  }

  std::vector<unsigned int> mem_version;
  mem_version.resize(stmt_info.size(), ~0);

  std::queue<unsigned int> queue;
  for (size_t i = stmt_info.size() - 1; ~i; --i)
  {
    if (degrees[i] != 0)
      continue;
    mem_version[i] = i == 0 ? 1 : 0;
    for (auto npos : stmt_info[i].next)
      if (npos > i && --degrees[npos] == 0)
        queue.push(npos);
  }

  unsigned int last_version = 1;

  std::unordered_map<size_t, size_t> stmt_to_loop;
  for (size_t i = 1; i < loops.size(); ++i)
    stmt_to_loop[loops[i].head] = i;

  while (!queue.empty())
  {
    unsigned int pos = queue.front();
    queue.pop();

    if (auto it = stmt_to_loop.find(pos);
        it != stmt_to_loop.end()) {
      const auto &loop = loops[it->second];
      bool found = false;
      for (auto stmt : loop.stmts)
      {
        if (!func->stmts[stmt]->maybe_mem_store())
          continue;
        found = true;
        break;
      }

      unsigned int oldv;
      assert(stmt_info[pos].prev.size() == 1);
      for (auto ppos : stmt_info[pos].prev)
        oldv = mem_version[ppos];
      assert(oldv != ~0u);

      if (!found)
        mem_version[++pos] = oldv;
      else
        mem_version[++pos] = ++last_version;
    } else if (func->stmts[pos]->maybe_mem_store()) {
      mem_version[pos] = ++last_version;
    } else {
      unsigned int oldv, cnt = 0;
      for (auto ppos : stmt_info[pos].prev)
      {
        unsigned int ver = mem_version[ppos];
        assert(ver != ~0u);
        if (cnt == 1 && (ver == 0 || oldv == ver)) {
          continue;
        } else if (cnt == 1 && oldv != 0) {
          cnt = 2;
          break;
        }
        oldv = ver;
        cnt = 1;
      }
      if (cnt > 1) {
        mem_version[pos] = ++last_version;
      } else {
        assert(cnt == 1);
        mem_version[pos] = oldv;
      }
    }

    for (auto npos : stmt_info[pos].next)
    {
      if (npos < pos)
        continue;

      assert(degrees[npos] > 0);
      if (--degrees[npos] == 0)
        queue.push(npos);
    }
  }

  Bitset def_live(func->num_temps);
  std::stack<std::pair<size_t, MirLocal>> stack;
  std::unordered_map<MirLocal, MirLocal> rules;
  std::unordered_map<MirCachedStmt,
    MirLocal, MirStmtHash, MirStmtEqual> cache;

  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    for (auto ppos : stmt_info[i].prev)
    {
      while (!stack.empty() && stack.top().first > ppos)
      {
        def_live.clr(stack.top().second);
        stack.pop();
      }
    }

    if (!func->stmts[i]->apply_rules(rules))
      continue;

    MirLocal local = func->stmts[i]->get_def();
    assert(local >= func->num_locals && local < num_phis);
    if (local >= func->num_temps)
      continue;

    unsigned int memver = 0;
    if (func->stmts[i]->is_mem_load()) {
      memver = mem_version[i];
      assert(memver != ~0u);
    }

    auto cached_stmt =
      std::make_pair(func->stmts[i].get(), memver);
    auto it = cache.find(cached_stmt);

    if (it != cache.end() && def_live.get(it->second)) {
      rules[func->stmts[i]->get_def()] = it->second;
    } else {
      cache[cached_stmt] = local;
      stack.push(std::make_pair(i, local));
      def_live.set(local);
    }
  }
}

void MirFuncContext::remove_unused(void)
{
  std::vector<size_t> tmp_definitions;
  std::vector<std::vector<size_t>> phi_definitions;

  tmp_definitions.resize(func->num_temps - func->num_locals);
  phi_definitions.resize(num_phis - func->num_temps);

  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    if (stmt_info[i].def == ~0u)
      continue;
    if (stmt_info[i].def < func->num_temps) {
      assert(stmt_info[i].def >= func->num_locals);
      tmp_definitions[stmt_info[i].def - func->num_locals] = i;
    } else {
      assert(stmt_info[i].def < num_phis);
      phi_definitions[stmt_info[i].def - func->num_temps].emplace_back(i);
    }
  }

  size_t num_temps = func->num_temps - func->num_locals;
  Bitset used_defs(num_phis - func->num_locals);
  std::queue<size_t> tmp_queue;
  std::queue<size_t> phi_queue;

  for (size_t i = 0; i < stmt_info.size(); ++i)
  {
    if (!func->stmts[i]->is_return()
        && !stmt_info[i].func_call
        && stmt_info[i].def != ~0u)
      continue;
    for (auto use : func->stmts[i]->get_uses())
    {
      if (use == ~0u)
        continue;
      if (use < func->num_args)
        continue;
      assert(use >= func->num_locals);
      use -= func->num_locals;

      if (used_defs.get(use))
        continue;
      used_defs.set(use);
      if (use < num_temps)
        tmp_queue.push(use);
      else
        phi_queue.push(use - num_temps);
    }
  }

  std::vector<size_t> unary_vec;
  unary_vec.resize(1);

  for (;;)
  {
    const std::vector<size_t> *vector;
    if (!tmp_queue.empty()) {
      size_t tmp = tmp_queue.front();
      tmp_queue.pop();

      unary_vec[0] = tmp_definitions[tmp];
      vector = &unary_vec;
    } else if (!phi_queue.empty()) {
      size_t phi = phi_queue.front();
      phi_queue.pop();

      vector = &phi_definitions[phi];
    } else {
      break;
    }

    for (auto stmt : *vector)
    {
      std::vector<MirLabel> uses = func->stmts[stmt]->get_uses();
      for (auto use : uses)
      {
        if (use == ~0u)
          continue;
        if (use < func->num_args)
          continue;
        assert(use >= func->num_locals);
        use -= func->num_locals;

        if (used_defs.get(use))
          continue;
        used_defs.set(use);
        if (use >= num_temps)
          phi_queue.push(use - num_temps);
        else
          tmp_queue.push(use);
      }
    }
  }

  Bitset removed_stmts(stmt_info.size());
  for (size_t i = 0; i < func->stmts.size(); ++i)
  {
    if (stmt_info[i].def == ~0u)
      continue;
    if (used_defs.get(stmt_info[i].def - func->num_locals))
      continue;
    if (stmt_info[i].func_call)
      func->stmts[i]->remove_dest();
    else
      removed_stmts.set(i);
  }

  std::vector<std::unique_ptr<MirStmt>> stmts;
  std::vector<size_t> labels;
  labels.resize(func->labels.size());

  std::vector<std::pair<size_t, MirLabel>> sorted_labels;
  for (unsigned int i = 0; i < func->labels.size(); ++i)
    sorted_labels.emplace_back(func->labels[i], i);
  std::sort(sorted_labels.begin(), sorted_labels.end());

  size_t j = 0;
  for (size_t i = 0; i < func->stmts.size(); ++i)
  {
    bool has_label =
      j < sorted_labels.size() && sorted_labels[j].first == i;
    bool put_stmt = !removed_stmts.get(i);

    assert(!has_label || put_stmt);

    if (has_label) {
      labels[sorted_labels[j].second] = stmts.size();
      ++j;
    }
    if (put_stmt)
      stmts.emplace_back(std::move(func->stmts[i]));
  }
  assert(j == sorted_labels.size());

  func->stmts = std::move(stmts);
  func->labels = std::move(labels);

  prepare();
}

void MirFuncContext::optimize(void)
{
  move_invariants();
  convert_all_to_ssa();
  merge_duplicates();
  remove_unused();
}

Bitset MirFuncContext::calc_reachable(void)
{
  Bitset reachable(stmt_info.size());
  std::queue<size_t> queue;
  queue.push(0);
  reachable.set(0);

  while (!queue.empty())
  {
    size_t pos = queue.front();
    queue.pop();
    for (size_t npos : stmt_info[pos].next)
    {
      if (!reachable.get(npos)) {
        reachable.set(npos);
        queue.push(npos);
      }
    }
  }

  return reachable;
}
