#ifdef __SYSY_TEST__

#include <stdio.h>

void foo(void);

int main(void)
{
  foo();

  puts(__FILE__ " passed");
  return 0;
}

#endif
