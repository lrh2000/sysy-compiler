#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

extern int counter;
int shortcut_or(int lhs);
int shortcut_and(int rhs);

int main(void)
{
  assert(counter == 0);

  assert(shortcut_or(1) == 1);
  assert(counter == 0);

  assert(shortcut_or(0) == 0);
  assert(counter == 1);

  assert(shortcut_and(1) == 1);
  assert(counter == 2);

  assert(shortcut_and(0) == 0);
  assert(counter == 2);

  puts(__FILE__ " passed");
  return 0;
}

#endif
