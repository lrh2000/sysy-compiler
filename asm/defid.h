#pragma once

typedef signed int AsmImm;

struct AsmLabelId
{
  const unsigned int id;

private:
  AsmLabelId(unsigned int id)
    : id(id)
  {}

  friend class AsmBuilder;
};
