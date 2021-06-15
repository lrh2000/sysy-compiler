#include <stdio.h>
#include <assert.h>

extern int a[];

int foo(void);

int main(void)
{
  for (int i = 0; i < 34; ++i)
    a[i] = 100 - i;

  foo();

  for (int i = 0; i < 33; ++i)
    assert(a[i] == i);

  puts(__FILE__ " passed");
  return 0;
}
