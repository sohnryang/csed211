/* Testing Code */

#include <limits.h>
#include <math.h>

/* Routines used by floation point test code */

int test_logicalShift(int x, int n) {
  unsigned u = (unsigned) x;
  unsigned shifted = u >> n;
  return (int) shifted;
}
int test_bitNor(int x, int y)
{
	return ~(x|y); 
}

int test_isZero(int x) {
  return x == 0;
}

int test_absVal(int x) {
  return (x < 0) ? -x : x;
}
int test_addOK(int x, int y)
{
	long long lsum = (long long) x + y;
	return lsum == (int) lsum;
}

