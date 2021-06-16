#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

int calc1(int x);
int calc2(int x);
int calc3(int x);
int calc4(int x);

int main(void)
{
#define TEST(v)                               \
  assert(calc1(v) == (v) + (v) * (v));        \
  assert(calc2(v) == (v) + (v) * (v));        \
  assert(calc3(v) == (v) + (v) * (v));        \
  assert(calc4(v) == (v) + (v) * (v));

  TEST(2333);
  TEST(6666);
  TEST(-1234);

  puts(__FILE__ " passed");
  return 0;
}

#endif
