#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

static int A[4][4] = {
  {  445, -411, -225,  480, },
  {  180,  -38,  434, -319, },
  { -184, -191,  422,  403, },
  {  385,  127,   83, -130, },
};

static int B[4][4] = {
  {  308, -493,  321, -266, },
  {  356, -351,  205, -463, },
  { -456, -206,  468, -308, },
  {  -44,  330,  205, -232, },
};

static int C[4][4];

static int D[4][4];

void matmul(int md[][4], int ms1[][4], int ms2[][4]);

int main(void)
{
  for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
      for (size_t k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[i][k];

  for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
      D[i][j] = A[i][j] + B[i][j];

  matmul(D, A, B);

  for (size_t i = 0; i < 4; ++i)
    for (size_t j = 0; j < 4; ++j)
      assert(C[i][j] == D[i][j]);

  puts(__FILE__ " passed");
  return 0;
}

#endif
