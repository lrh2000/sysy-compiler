#pragma once
#include <cassert>
#include <ostream>

#define NR_REG_CALLER    16
#define NR_REG_CALLEE    13
#define NR_REGISTERS     29

#define MASK_REG_CALLER  0x0000ffff
#define MASK_REG_CALLEE  0x1fff0000
#define MASK_REGISTERS   0x1fffffff

enum class Register
{
  T0, T1, T2, T3, T4, T5, T6,
  RA,
  A0, A1, A2, A3, A4, A5, A6, A7,
  S0, S1, S2, S3, S4, S5, S6, S7, S8, S9, S10, S11,
  TP, GP, SP,
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

extern inline Register reg_from_callee_id(uint32_t id)
{
  return static_cast<Register>(id + static_cast<uint32_t>(Register::S0));
}

extern inline Register reg_from_arg_id(uint32_t id)
{
  return static_cast<Register>(id + static_cast<uint32_t>(Register::RA));
}
