#pragma once
#include <cassert>
#include <vector>
#include <unordered_map>
#include "../lexer/symbol.h"
#include "defid.h"
#include "../hir/defid.h"

enum class AstDefKind
{
  Func,
  Const,
  Array,
};

class AstType;
class AstFuncType;

class AstScope
{
public:
  AstScope(void)
    : symbols(), values(), arrays(),
      locals(), globals(), types()
  {}

  AstScope(AstScope &&other) = default;
  AstScope(const AstScope &other) = delete;
  AstScope &operator =(const AstScope &other) = delete;

  bool def_insert(Symbol name, AstLocalDefId &id)
  {
    auto res = symbols.emplace(name, symbols.size());
    id = res.first->second;
    return res.second;
  }

  bool def_lookup(Symbol name, AstLocalDefId &id) const
  {
    auto it = symbols.find(name);
    if (it == symbols.end())
      return false;
    id = it->second;
    return true;
  }

  void def_set_type(AstLocalDefId id, const AstType *ty)
  {
    assert(id == types.size());
    types.emplace_back(ty);
  }

  const AstType *def_get_type(AstLocalDefId id) const
  {
    assert(id < types.size());
    return types[id];
  }

  void def_set_value(AstLocalDefId id, int value)
  {
    auto res = values.emplace(id, value);
    assert(res.second == true);
  }

  int def_get_value(AstLocalDefId id) const
  {
    auto it = values.find(id);
    assert(it != values.end());
    return it->second;
  }

  void def_set_arrayid(AstLocalDefId id, HirArrayId hirid)
  {
    auto res = arrays.emplace(id, hirid);
    assert(res.second == true);
  }

  HirArrayId def_get_arrayid(AstLocalDefId id) const
  {
    auto it = arrays.find(id);
    assert(it != arrays.end());
    return it->second;
  }

  void def_set_localid(AstLocalDefId id, HirLocalId hirid)
  {
    auto res = locals.emplace(id, hirid);
    assert(res.second == true);
  }

  HirLocalId def_get_localid(AstLocalDefId id) const
  {
    auto it = locals.find(id);
    assert(it != locals.end());
    return it->second;
  }

  void def_set_symbol(AstLocalDefId id, Symbol symbol)
  {
    auto res = globals.emplace(id, symbol);
    assert(res.second == true);
  }

  Symbol def_get_symbol(AstLocalDefId id) const
  {
    auto it = globals.find(id);
    assert(it != globals.end());
    return it->second;
  }

private:
  std::unordered_map<Symbol, AstLocalDefId> symbols;
  std::unordered_map<AstLocalDefId, int> values;
  std::unordered_map<AstLocalDefId, HirArrayId> arrays;
  std::unordered_map<AstLocalDefId, HirLocalId> locals;
  std::unordered_map<AstLocalDefId, Symbol> globals;
  std::vector<const AstType *> types;
};

class AstContext
{
public:
  AstContext(void)
    : scopes(), scope_stack(), func_ty(nullptr), nested_loops(0)
  {}

  AstContext(AstContext &&other) = default;
  AstContext(const AstContext &other) = delete;
  AstContext &operator =(const AstContext &other) = delete;

  void scope_push(void)
  {
    scope_stack.emplace_back(scopes.size());
    scopes.emplace_back();
  }

  AstScopeId scope_top(void) const
  {
    assert(scope_stack.size() > 0);
    return scope_stack[scope_stack.size() - 1];
  }

  void scope_pop(void)
  {
    assert(scope_stack.size() > 0);
    scope_stack.pop_back();
  }

  AstDefId def_insert(Symbol name)
  {
    AstLocalDefId id;
    bool ok = scopes[scope_top()].def_insert(name, id);
    if (!ok)
      return AstDefId();
    return AstDefId(scope_top(), id);
  }

  AstDefId def_lookup(Symbol name) const
  {
    for (size_t i = scope_stack.size() - 1; ~i; --i)
    {
      AstScopeId sid = scope_stack[i];
      AstLocalDefId id;
      if (scopes[sid].def_lookup(name, id))
        return AstDefId(sid, id);
    }
    return AstDefId();
  }

  void def_set_type(AstDefId id, const AstType *type)
  {
    scopes[id.scope].def_set_type(id.id, type);
  }

  const AstType *def_get_type(AstDefId id) const
  {
    return scopes[id.scope].def_get_type(id.id);
  }

  void def_set_value(AstDefId id, int value)
  {
    scopes[id.scope].def_set_value(id.id, value);
  }

  int def_get_value(AstDefId id) const
  {
    return scopes[id.scope].def_get_value(id.id);
  }

  void def_set_arrayid(AstDefId id, HirArrayId value)
  {
    scopes[id.scope].def_set_arrayid(id.id, value);
  }

  HirArrayId def_get_arrayid(AstDefId id) const
  {
    return scopes[id.scope].def_get_arrayid(id.id);
  }

  void def_set_localid(AstDefId id, HirLocalId value)
  {
    scopes[id.scope].def_set_localid(id.id, value);
  }

  HirLocalId def_get_localid(AstDefId id) const
  {
    return scopes[id.scope].def_get_localid(id.id);
  }

  void def_set_symbol(AstDefId id, Symbol value)
  {
    scopes[id.scope].def_set_symbol(id.id, value);
  }

  Symbol def_get_symbol(AstDefId id) const
  {
    return scopes[id.scope].def_get_symbol(id.id);
  }

  void func_push(const AstFuncType *ty)
  {
    assert(func_ty == nullptr);
    func_ty = ty;
  }

  const AstFuncType *func_top(void) const
  {
    assert(func_ty != nullptr);
    return func_ty;
  }

  void func_pop(void)
  {
    assert(func_ty != nullptr);
    func_ty = nullptr;
  }

  void loop_push(void)
  {
    ++nested_loops;
  }

  bool loop_check(void)
  {
    return nested_loops != 0;
  }

  void loop_pop(void)
  {
    assert(nested_loops > 0);
    --nested_loops;
  }

private:
  std::vector<AstScope> scopes;
  std::vector<AstScopeId> scope_stack;

  const AstFuncType *func_ty;

  unsigned int nested_loops;
};
