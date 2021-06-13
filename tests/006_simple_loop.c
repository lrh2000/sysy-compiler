#include <stdio.h>
#include <assert.h>

int sum(int n);

int main(void)
{
  int s = 0;
  for (int i = 0; i <= 100; ++i)
  {
    s += i;
    assert(sum(i) == s);
  }

  puts(__FILE__ " passed");
  return 0;
}
