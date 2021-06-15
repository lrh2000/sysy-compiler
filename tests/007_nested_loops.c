#include <stdio.h>
#include <assert.h>

int next_prime(int x, int n);

int main(void)
{
  assert(next_prime(190, 200) == 191);
  assert(next_prime(191, 200) == 193);
  assert(next_prime(192, 200) == 193);
  assert(next_prime(193, 200) == 197);
  assert(next_prime(194, 200) == 197);
  assert(next_prime(195, 200) == 197);
  assert(next_prime(196, 200) == 197);
  assert(next_prime(197, 200) == 199);
  assert(next_prime(198, 200) == 199);
  assert(next_prime(199, 200) == 200);
  assert(next_prime(200, 200) == 200);

  puts(__FILE__ " passed");
  return 0;
}
