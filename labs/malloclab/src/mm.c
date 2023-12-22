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
#define CHUNKSIZE (1 << 12)

/* header - header of a block. */
struct header {
  uint32_t size_with_flags;
  struct header *prev;
  struct header *next;
};

/* DEREF_WORD - dereference word in `addr`. */
#define DEREF_WORD(addr) (*((word_t *)addr))

/* BLOCK_HEADER - get the header of a block in `addr`. */
#define BLOCK_HEADER(addr) (((word_t *)addr) - 1)

/* HEADER_SIZE - get the size of a block header. */
#define HEADER_SIZE(header) ((header) & ~3)

/* HEADER_INUSE - get the inuse bit of a block header. */
#define HEADER_INUSE(header) (((header) & 2) >> 1)

/* HEADER_PREVINUSE - get the prev_inuse bit of a block header. */
#define HEADER_PREVINUSE(header) ((header) & 1)

/* PACK_SIZE - pack `size`, `inuse`, `prev_inuse` into a word. */
#define PACK_SIZE(size, inuse, prev_inuse)                                     \
  ((size) | ((inuse) << 1) | (prev_inuse))

static struct header *freelist;

/*
 * expand_heap - expand heap by `words` words.
 * Returns a pointer to a block created by expansion. Returns NULL if sbrk
 * failed.
 */
static word_t *expand_heap(size_t words) {
  size_t expand_words = (words % 2 ? words + 1 : words),
         expand_size = WORDSIZE * expand_words;
  void *old_brk = mem_sbrk(expand_size);
  word_t *new_block, epilogue;

  if (old_brk == (void *)-1)
    return NULL;

  epilogue = DEREF_WORD(old_brk);
  new_block = old_brk;
  new_block[0] = PACK_SIZE(expand_words, 0, HEADER_PREVINUSE(epilogue));
  new_block[expand_words - 1] = new_block[0];
  new_block[expand_words] = PACK_SIZE(0, 1, 0);
  return new_block;
}

/*
 * find_best_fit - find the best fit block in the free list.
 * Returns a pointer to a block with smallest size that is at least `size`
 * bytes.
 */
static word_t *find_best_fit(size_t size) {
  struct header *current_best, *current;

  if (freelist == NULL)
    return NULL;

  current_best = freelist;
  for (current = freelist; current != NULL; current = current->next) {
    if (HEADER_SIZE(current_best->size_with_flags) < size)
      continue;

    if (HEADER_SIZE(current->size_with_flags) <
        HEADER_SIZE(current_best->size_with_flags))
      current_best = current;
  }

  if (HEADER_SIZE(current_best->size_with_flags) < size)
    return NULL;
  else
    return (word_t *)current_best;
}

/*
 * split_block - split a block into two blocks.
 * The first block has size `size` and the second block has the remaining size.
 * Returns a pointer to the second block.
 */
static word_t *split_block(word_t *block, size_t size) {
  size_t block_size = HEADER_SIZE(block[0]);
  word_t *new_block;
  int prev_inuse = HEADER_PREVINUSE(block[0]);

  if (block_size < size + 2)
    return block;

  block[0] = PACK_SIZE(size, 0, prev_inuse);
  block[size - 1] = block[0];
  block[size] = PACK_SIZE(block_size - size, 0, 0);
  block[block_size - 1] = block[size];
  return block + size;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  word_t *prologue_block, *init_block;
  struct header *init_block_header;

  prologue_block = mem_sbrk(4 * WORDSIZE);
  if (prologue_block == (void *)-1)
    return -1;

  prologue_block[0] = 0;
  prologue_block[1] = PACK_SIZE(8, 1, 0);
  prologue_block[2] = PACK_SIZE(8, 1, 0);
  prologue_block[3] = PACK_SIZE(0, 1, 1);

  init_block = expand_heap(CHUNKSIZE / WORDSIZE);
  if (init_block == NULL)
    return -1;

  init_block_header = (struct header *)init_block;
  init_block_header->prev = NULL;
  init_block_header->next = NULL;
  freelist = init_block_header;
  return 0;
}

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
