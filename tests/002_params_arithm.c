#ifdef __SYSY_TEST__

#include <stdio.h>
#include <assert.h>

int add(int a, int b);
int sub(int a, int b);
int mul(int a, int b);
int div(int a, int b);

int main(void)
{
#define TEST(a, b)                  \
  assert(add(a, b) == a + b);       \
  assert(sub(a, b) == a - b);       \
  assert(mul(a, b) == a * b);       \
  assert(div(a, b) == a / b);

  TEST(65432, 12345);
  TEST(65432, -12345);
  TEST(-98765, 15678);
  TEST(-98765, -15678);

  puts(__FILE__ " passed");
  return 0;
}

#endif
