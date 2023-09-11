#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define TMin INT_MIN
#define TMax INT_MAX

#include "btest.h"
#include "bits.h"

test_rec test_set[] = {




 {"bitNor", (funct_t) bitNor, (funct_t) test_bitNor, 2, "~ &", 8, 1,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},


 {"isZero", (funct_t) isZero, (funct_t) test_isZero, 2, "! ~ & ^ | + << >>", 2, 1,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},


 {"addOK", (funct_t) addOK, (funct_t) test_addOK, 2,
	 "! ~ & ^ | + << >>", 20,3,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},


 {"absVal", (funct_t) absVal, (funct_t) test_absVal, 2,
     "! ~ & ^ | + << >>", 10,4,
  {{TMin, TMax},{TMin,TMax},{TMin,TMax}}},


 {"logicalShift", (funct_t) logicalShift, (funct_t) test_logicalShift,
   2, "! ~ & ^ | + << >>", 20, 3,
  {{TMin, TMax},{0,31},{TMin,TMax}}},



  {"", NULL, NULL, 0, "", 0, 0,
   {{0, 0},{0,0},{0,0}}}
};
