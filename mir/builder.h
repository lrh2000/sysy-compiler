#pragma once
#include <cassert>
#include <stack>
#include <vector>
#include <memory>
#include "mir.h"

class MirFuncBuilder
{
public:
  MirFuncBuilder(Symbol name, size_t num_args, size_t num_locals,
      size_t array_size, std::vector<size_t> &&array_offs)
    : labels(), stmts(), loop_heads(), loop_tails(),
      num_args(num_args), num_locals(num_locals),
      num_temps(num_locals), array_size(array_size),
      array_offs(std::move(array_offs)), name(name)
  {
    stmts.emplace_back(std::make_unique<MirEmptyStmt>());
    stmts.emplace_back(std::make_unique<MirEmptyStmt>());
  }

  MirLabel new_label(void)
  {
    labels.push_back(0xdeadbeef);
    return labels.size() - 1;
  }

  void set_label(MirLabel label)
  {
    labels[label] = stmts.size();
    stmts.emplace_back(std::make_unique<MirEmptyStmt>());
  }

  MirLocal new_temp(void)
  {
    return num_temps++;
  }

  void add_statement(std::unique_ptr<MirStmt> &&stmt)
  {
    stmts.emplace_back(std::move(stmt));
  }

  void finish_build(void)
  {
    set_label(new_label());
  }

  void loop_push(MirLabel head, MirLabel tail)
  {
    loop_heads.push(head);
    loop_tails.push(tail);
    stmts.emplace_back(std::make_unique<MirEmptyStmt>());
  }

  MirLabel loop_get_head(void)
  {
    assert(loop_heads.size() > 0);
    return loop_heads.top();
  }

  MirLabel loop_get_tail(void)
  {
    assert(loop_tails.size() > 0);
    return loop_tails.top();
  }

  void loop_pop(void)
  {
    loop_heads.pop();
    loop_tails.pop();
  }

private:
  std::vector<size_t> labels;
  std::vector<std::unique_ptr<MirStmt>> stmts;

  std::stack<MirLabel> loop_heads;
  std::stack<MirLabel> loop_tails;

  size_t num_args;
  size_t num_locals;
  size_t num_temps;

  size_t array_size;
  std::vector<size_t> array_offs;

  Symbol name;

  friend class MirFuncItem;
};

inline MirFuncItem::MirFuncItem(MirFuncBuilder &&builder)
  : name(builder.name), labels(std::move(builder.labels)),
    stmts(std::move(builder.stmts)),
    num_args(builder.num_args), num_locals(builder.num_locals),
    num_temps(builder.num_temps), array_size(builder.array_size),
    array_offs(std::move(builder.array_offs))
{}

class MirBuilder
{
public:
  MirBuilder(void)
    : items()
  {}

  void add_item(std::unique_ptr<MirItem> &&item)
  {
    items.emplace_back(std::move(item));
  }

private:
  std::vector<std::unique_ptr<MirItem>> items;

  friend class MirCompUnit;
};

inline MirCompUnit::MirCompUnit(MirBuilder &&builder)
  : items(std::move(builder.items))
{}
