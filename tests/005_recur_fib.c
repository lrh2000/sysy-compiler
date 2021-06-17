#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

int fib(int x);

int main(void)
{
  assert(fib(0) == 0);
  assert(fib(1) == 1);
  assert(fib(2) == 1);
  assert(fib(3) == 2);
  assert(fib(4) == 3);
  assert(fib(5) == 5);
  assert(fib(6) == 8);
  assert(fib(7) == 13);
  assert(fib(8) == 21);
  assert(fib(9) == 34);
  assert(fib(10) == 55);
  assert(fib(11) == 89);

  puts(__FILE__ " passed");
  return 0;
}

#endif
