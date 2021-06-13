#include <stdio.h>
#include <assert.h>

static int a[5][4] = {{}, 1, 2, 3, 4, {}, {5, 6, 7}, 8, 9};

int get1(int x, int y);
int get2(int x, int y);

__attribute__ ((noinline))
void put_random_bits(void)
{
  volatile int b[30];

  for (int i = 0; i < 30; ++i)
    b[i] = 0xdeadbeef;
}

int main(void)
{
  put_random_bits();

  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 4; ++j)
      assert(get1(i, j) == a[i][j]);

  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 4; ++j)
      assert(get2(i, j) == a[i][j]);

  puts(__FILE__ " passed");
  return 0;
}
