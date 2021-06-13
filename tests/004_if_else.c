#include <stdio.h>
#include <assert.h>

int min1(int a, int b);
int min2(int a, int b);

int main(void)
{
#define TEST(a, b)                                   \
  assert(min1(a, b) == ((a) < (b) ? (a) : (b)));       \
  assert(min2(a, b) == ((a) < (b) ? (a) : (b)));

  TEST(233, 666);
  TEST(666, 233);

  puts(__FILE__ " passed");
  return 0;
}
