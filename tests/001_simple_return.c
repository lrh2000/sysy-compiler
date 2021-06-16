#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

int foo(void);
int bar(void);

int main(void)
{
  assert(foo() == 0);
  assert(bar() == -175);

  puts(__FILE__ " passed");
  return 0;
}

#endif
