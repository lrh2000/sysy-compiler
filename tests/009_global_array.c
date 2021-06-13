#include <stdio.h>
#include <assert.h>

static int array[5][3] = {1, 2, 3, {4}, {}, {5}, 6, 7};

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
