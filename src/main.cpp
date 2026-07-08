#define X(...) TEST(__VA_ARGS__)
#define TEST(name, value) int name = value;

X(x, 5)

#include "stdlib.h"

int main()
{
  printf("X: %d \n", x);

  return 0;
}