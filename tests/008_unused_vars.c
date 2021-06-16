#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

int foo(int x, int y);

int main(void)
{
  assert(foo(0, 1) == -1);

  puts(__FILE__ " passed");
  return 0;
}

#endif
