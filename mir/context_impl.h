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
