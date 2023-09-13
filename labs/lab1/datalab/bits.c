/*
 *
 * CSED 211, Fall 2022
 * DataLAb: Manipulating Bits
 * Assigned: 2022-09-07
 * Due: 2022-09-18 23:59 pm
 *
 * Namgyu Park (namgyu.park@postech.ac.kr) is the lead person for this assignment.
 *
*/


#if 0
LAB L1 INSTRUCTIONS:
#endif

/*
 *   #Homework1-1 
 *   bitNor - ~(x|y) using only ~ and & 
 *   Example: bitNor(0x6, 0x5) = 0xFFFFFFF8
 *   Legal ops: ~ &
 *   Max ops: 8
 *   Rating: 1
 */

int bitNor(int x, int y) {
  return (~x) & (~y);
}


/*
 *   #Homework1-2
 *   isZero - returns 1 if x == 0, and 0 otherwise 
 *   Examples: isZero(5) = 0, isZero(0) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 2
 *   Rating: 1
 */
int isZero(int x) {
  return !x;
}


/* 
 *   #Homework1-3
 *   addOK - Determine if can compute x+y without overflow
 *   Example: addOK(0x80000000,0x80000000) = 0,
 *            addOK(0x80000000,0x70000000) = 1, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int addOK(int x, int y) {
  int x_top2 = (x >> 30) & 3, y_top2 = (y >> 30) & 3;
  int carry_tail = ((x ^ (x_top2 << 30)) + (y ^ (y_top2 << 30))) >> 30;
  int carry_in_unshifted = (x_top2 & 1) + (y_top2 & 1) + carry_tail;
  return ((x_top2 ^ y_top2) >> 1) | !((carry_in_unshifted ^ x_top2) >> 1);
}


/*   
 *   #Homework1-4
 *   absVal - absolute value of x
 *   Example: absVal(-1) = 1.
 *   You may assume -TMax <= x <= TMax
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 10
 *   Rating: 4
 */
int absVal(int x) {
  return ((((1 << 31) & x) >> 31) ^ x) + !!((1 << 31) & x);
}


/*   
 *   #Homework1-5
 *   logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  return ((((1 << 31) & x) >> n) << 1) ^ (x >> n);
}

