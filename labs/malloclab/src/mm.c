/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) < (y)) ? (y) : (x))

typedef uint32_t word_t;
#define WORDSIZE sizeof(word_t)

/* DEREF_WORD - dereference word in `addr`. */
#define DEREF_WORD(addr) (*((word_t *)addr))

/* BLOCK_HEADER - get the header of a block in `addr`. */
#define BLOCK_HEADER(addr) (((word_t *)addr) - 1)

/* HEADER_INUSE - get the inuse bit of a block header. */
#define HEADER_INUSE(header) (((header) & 2) >> 1)

/* HEADER_PREVINUSE - get the prev_inuse bit of a block header. */
#define HEADER_PREVINUSE(header) ((header) & 1)

/* PACK_SIZE - pack `size`, `inuse`, `prev_inuse` into a word. */
#define PACK_SIZE(size, inuse, prev_inuse)                                     \
  ((size) | ((inuse) << 1) | (prev_inuse))

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) { return 0; }

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  int newsize = ALIGN(size + SIZE_T_SIZE);
  void *p = mem_sbrk(newsize);
  if (p == (void *)-1)
    return NULL;
  else {
    *(size_t *)p = size;
    return (void *)((char *)p + SIZE_T_SIZE);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}