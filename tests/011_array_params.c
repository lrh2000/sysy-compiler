#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

static int array[5][3][4];

int get_3d(int a[][3][4], int x, int y, int z);

int main(void)
{
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 4; ++k)
        array[i][j][k] = (i << 8) | (j << 4) | (k << 0);

  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 4; ++k)
        assert(get_3d(array, i, j, k) == array[i][j][k]);

  puts(__FILE__ " passed");
  return 0;
}

#endif
