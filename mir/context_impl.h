#pragma once
#include "context.h"
#include "../utils/bitset.h"

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
