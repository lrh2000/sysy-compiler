#pragma once
#include <cassert>
#include <ostream>

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


