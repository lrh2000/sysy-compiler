#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
static int array[5][3] = {1, 2, 3, {4}, {}, {5}, 6, 7};
#pragma GCC diagnostic pop

void set(int x, int y, int z);
int get(int x, int y);

int main(void)
{
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 3; ++j)
      assert(get(i, j) == array[i][j]);

  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 3; ++j)
      set(i, j, (i ^ 233) * (j ^ 666));

  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 3;++j)
      assert(get(i, j) == (i ^ 233) * (j ^ 666));

  puts(__FILE__ " passed");
  return 0;
}

#endif
