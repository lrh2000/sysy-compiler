#pragma once
#include <cstddef>
#include <cstdint>
#include <cassert>
#include <vector>

class Bitset;

class BitsetIterator
{
public:
  BitsetIterator &operator ++(void);

  size_t operator *(void) const
  {
    return position;
  }

  bool operator !=(const BitsetIterator &other)
  {
    return position != other.position;
  }

private:
  BitsetIterator(const Bitset *self, bool end);

private:
  const Bitset *self;
  size_t position;
  uint64_t current;

  friend class Bitset;
};

class Bitset
{
public:
  Bitset(size_t num_bits)
    : num_bits(num_bits), data()
  {
    data.resize((num_bits + 63) / 64, 0);
  }

  Bitset(Bitset &&other)
    : num_bits(other.num_bits), data(std::move(other.data))
  {
    other.num_bits = 0;
  }

  Bitset(const Bitset &other)
    : num_bits(other.num_bits), data(other.data)
  {}

  Bitset &operator =(const Bitset &other)
  {
    num_bits = other.num_bits;
    data = other.data;
    return *this;
  }

  bool get(size_t pos) const
  {
    return 1 & (data[pos / 64] >> (pos % 64));
  }

  void set(size_t pos)
  {
    data[pos / 64] |= 1ull << (pos % 64);
  }

  void clr(size_t pos)
  {
    data[pos / 64] &= ~(1ull << (pos % 64));
  }

  void set_all(void)
  {
    for (size_t i = 0; i < num_bits / 64; ++i)
      data[i] = ~0ull;
    if (num_bits % 64)
      data[num_bits / 64] = (1ull << (num_bits % 64)) - 1;
  }

  void reset(void)
  {
    for (size_t i = 0; i < data.size(); ++i)
      data[i] = 0;
  }

  bool test(const Bitset &other) const
  {
    assert(num_bits == other.num_bits);
    for (size_t i = 0; i < data.size(); ++i)
      if (data[i] & other.data[i])
        return true;
    return false;
  }

  bool contain(const Bitset &other) const
  {
    assert(num_bits == other.num_bits);
    for (size_t i = 0; i < data.size(); ++i)
      if ((data[i] & other.data[i]) != other.data[i])
        return false;
    return true;
  }

  Bitset &operator |=(const Bitset &other)
  {
    assert(num_bits == other.num_bits);
    for (size_t i = 0; i < data.size(); ++i)
      data[i] |= other.data[i];
    return *this;
  }

  Bitset &operator -=(const Bitset &other)
  {
    assert(num_bits == other.num_bits);
    for (size_t i = 0; i < data.size(); ++i)
      data[i] &= ~other.data[i];
    return *this;
  }

  BitsetIterator begin(void) const
  {
    return BitsetIterator(this, false);
  }

  BitsetIterator end(void) const
  {
    return BitsetIterator(this, true);
  }

private:
  size_t num_bits;
  std::vector<uint64_t> data;

  friend class BitsetIterator;
};

inline BitsetIterator::BitsetIterator(const Bitset *self, bool end)
  : self(self),
    position(end ? self->num_bits : 0),
    current(self->num_bits ? self->data[0] : 0)
{
  if (!end)
    ++*this;
}

inline BitsetIterator &BitsetIterator::operator ++(void)
{
  while (current == 0)
  {
    (position &= ~(size_t) 63) += 64;
    if (position >= self->num_bits) {
      position = self->num_bits;
      return *this;
    }
    current = self->data[position / 64];
  }

  size_t tzcnt = __builtin_ctzll(current);
  (position &= ~(size_t) 63) += tzcnt;
  current -= 1ul << tzcnt;
  return *this;
}
