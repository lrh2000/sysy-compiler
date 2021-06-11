#pragma once
#include <stack>
#include <memory>
#include <vector>
#include "hir.h"

class HirFuncBuilder
{
public:
  HirFuncBuilder(size_t num_args)
    : array_num(0), array_maxsz(0), array_cursz(0),
      array_off(), array_stk(), num_args(num_args + 1),
      num_locals(1), body(), items()
  {
    body.push(std::vector<std::unique_ptr<HirStmt>>());
  }

  void scope_push(void)
  {
    array_stk.push(array_cursz);

    body.push(std::vector<std::unique_ptr<HirStmt>>());
  }

  std::unique_ptr<HirBlockStmt> scope_pop(void)
  {
    array_cursz = array_stk.top();
    array_stk.pop();

    auto stmt = std::make_unique<HirBlockStmt>(std::move(body.top()));
    body.pop();
    return stmt;
  }

  HirArrayId new_array(size_t size)
  {
    array_off.emplace_back(array_cursz);
    array_cursz += size;
    if (array_cursz > array_maxsz)
      array_maxsz = array_cursz;
    return array_off.size() - 1;
  }

  HirLocalId new_local()
  {
    return num_locals++;
  }

  void add_statement(std::unique_ptr<HirStmt> &&stmt)
  {
    body.top().emplace_back(std::move(stmt));
  }

  void add_item(std::unique_ptr<HirItem> &&item)
  {
    items.emplace_back(std::move(item));
  }

private:
  size_t array_num;
  size_t array_maxsz;
  size_t array_cursz;
  std::vector<size_t> array_off;
  std::stack<size_t> array_stk;

  size_t num_args;
  size_t num_locals;

  std::stack<std::vector<std::unique_ptr<HirStmt>>> body;

  std::vector<std::unique_ptr<HirItem>> items;

  friend class HirFuncItem;
};

inline HirFuncItem::HirFuncItem(HirFuncBuilder &&builder, Symbol name)
  : body(std::make_unique<HirBlockStmt>(std::move(builder.body.top()))),
    num_args(builder.num_args), num_locals(builder.num_locals),
    array_sz(builder.array_maxsz), array_off(std::move(builder.array_off)),
    items(std::move(builder.items)), name(name)
{
  builder.body.pop();
}
