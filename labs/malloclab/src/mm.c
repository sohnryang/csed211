/*
 * mm.c - Explicit free list with best-fit strategy and coalescing.
 *
 * In this approach, each block has a header and a footer. The header contains
 * the size of the block, the inuse bit, and the prev_inuse bit. The footer
 * contains the size of the block. The inuse bit indicates whether the block is
 * in use or not. The prev_inuse bit indicates whether the previous block is in
 * use or not. The header and the footer are both 4 bytes. The minimum block
 * size is 16 bytes, as the block should contain a header, a footer, and two
 * pointers for free list. For allocated blocks, space for pointers and footer
 * are used for payload.
 *
 * The free list is implemented as a doubly linked list. Each block contains
 * two pointers for the free list. The free list is oranized in LIFO order. When
 * searching for a free block, the best-fit strategy is used.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

team_t team = {"", "", "", "", ""};

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

/* DEREF - dereference word in `addr`. */
#define DEREF(addr) (*((word_t *)addr))

/* DEREF_FROM_NTH - dereference word in `addr` from `n`th byte. */
#define DEREF_FROM_NTH(addr, n) (*((word_t *)((uint8_t *)addr + n)))

/* HEADER_SIZE - get the size of a block header. */
#define HEADER_SIZE(header) ((header) & ~3)

/* HEADER_INUSE - get the inuse bit of a block header. */
#define HEADER_INUSE(header) (((header) & 2) >> 1)

/* HEADER_INUSE_SET - set the inuse bit of a block header. */
#define HEADER_INUSE_SET(header) ((header) |= 2)

/* HEADER_INUSE_CLEAR - clear the inuse bit of a block header. */
#define HEADER_INUSE_CLEAR(header) ((header) &= ~2)

/* HEADER_PREVINUSE - get the prev_inuse bit of a block header. */
#define HEADER_PREVINUSE(header) ((header) & 1)

/* HEADER_PREVINUSE_SET - set the prev_inuse bit of a block header. */
#define HEADER_PREVINUSE_SET(header) ((header) |= 1)

/* HEADER_PREVINUSE_CLEAR - clear the prev_inuse bit of a block header. */
#define HEADER_PREVINUSE_CLEAR(header) ((header) &= ~1)

/* FOOTER - get the pointer to footer of a block. */
#define FOOTER(addr) ((word_t *)((uint8_t *)addr + HEADER_SIZE(*addr) - 4))

/* NEXT_BLOCK - get the pointer to next block of a block. */
#define NEXT_BLOCK(block) ((word_t *)((uint8_t *)block + HEADER_SIZE(*block)))

/* PREV_FOOTER - get the pointer to footer of previous block of a block. */
#define PREV_FOOTER(block) ((word_t *)((uint8_t *)block - WORDSIZE))

/* PREV_BLOCK - get the pointer to previous block of a block. */
#define PREV_BLOCK(block)                                                      \
  ((word_t *)((uint8_t *)block - HEADER_SIZE(*PREV_FOOTER(block))))

/* PACK_SIZE - pack `size`, `inuse`, `prev_inuse` into a word. */
#define PACK_SIZE(size, inuse, prev_inuse)                                     \
  ((size) | ((inuse) << 1) | (prev_inuse))

/* PAYLOAD_HEADER - get the pointer to header of a block from its payload. */
#define PAYLOAD_HEADER(payload) ((word_t *)(payload)-1)

static struct header *freelist_head, *freelist_tail;

/*
 * list_insert - insert a block into the free list.
 * The block is inserted in the front of the list.
 */
static void list_insert(struct header *block) {
  block->prev = NULL;
  block->next = freelist_head;
  if (freelist_head != NULL)
    freelist_head->prev = block;
  freelist_head = block;
  if (freelist_tail == NULL)
    freelist_tail = block;
}

/*
 * list_remove - remove a block from the free list.
 */
static void list_remove(struct header *block) {
  if (block->prev != NULL)
    block->prev->next = block->next;
  if (block->next != NULL)
    block->next->prev = block->prev;
  if (block == freelist_head)
    freelist_head = block->next;
  if (block == freelist_tail)
    freelist_tail = block->prev;
}

/*
 * expand_heap - expand heap by `words` words.
 * Returns a pointer to a block created by expansion. Returns NULL if sbrk
 * failed.
 */
static word_t *expand_heap(size_t words) {
  size_t expand_words = (words % 2 ? words + 1 : words),
         expand_size = WORDSIZE * expand_words;
  void *new_block_payload = mem_sbrk(expand_size);
  word_t *new_block, epilogue;

  if (new_block_payload == (void *)-1)
    return NULL;

  epilogue = DEREF(PAYLOAD_HEADER(new_block_payload));
  new_block = PAYLOAD_HEADER(new_block_payload);
  DEREF_FROM_NTH(new_block, 0) =
      PACK_SIZE(expand_size, 0, HEADER_PREVINUSE(epilogue));
  DEREF_FROM_NTH(new_block, expand_size - WORDSIZE) = expand_size;
  DEREF_FROM_NTH(new_block, expand_size) = PACK_SIZE(0, 1, 0);
  return new_block;
}

/*
 * find_best_fit - find the best fit block in the free list.
 * Returns a pointer to a block with smallest size that is at least `size`
 * bytes.
 */
static word_t *find_best_fit(size_t size) {
  struct header *current_best, *current;

  if (freelist_head == NULL)
    return NULL;

  current_best = freelist_head;
  for (current = freelist_head; current != NULL; current = current->next) {
    if (HEADER_SIZE(current->size_with_flags) < size)
      continue;

    if (HEADER_SIZE(current_best->size_with_flags) < size ||
        HEADER_SIZE(current->size_with_flags) <
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
  int prev_inuse = HEADER_PREVINUSE(block[0]);

  if (block_size < size + 2)
    return block;

  DEREF_FROM_NTH(block, 0) = PACK_SIZE(size, 0, prev_inuse);
  DEREF_FROM_NTH(block, size - WORDSIZE) = size;
  DEREF_FROM_NTH(block, size) = PACK_SIZE(block_size - size, 0, 0);
  DEREF_FROM_NTH(block, block_size - WORDSIZE) = block_size - size;
  return &DEREF_FROM_NTH(block, size);
}

/*
 * coalesce_block - coalesce a block with its neighbors.
 * Returns a pointer to the coalesced block.
 */
static word_t *coalesce_block(word_t *block) {
  word_t *prev_block, *next_block = NEXT_BLOCK(block);
  size_t original_size = HEADER_SIZE(DEREF(block)),
         current_size = original_size,
         next_size = HEADER_SIZE(DEREF(next_block)), prev_size;
  int inuse = HEADER_INUSE(DEREF(block)),
      prev_inuse = HEADER_PREVINUSE(DEREF(block)), prev_prev_inuse;

  if (!HEADER_INUSE(DEREF(next_block))) {
    list_remove((struct header *)next_block);
    DEREF(block) = PACK_SIZE(original_size + next_size, inuse, prev_inuse);
    DEREF_FROM_NTH(block, original_size + next_size - WORDSIZE) =
        original_size + next_size;
    current_size += next_size;
  }

  if (prev_inuse)
    return block;

  prev_block = PREV_BLOCK(block);
  prev_size = HEADER_SIZE(DEREF(prev_block));
  list_remove((struct header *)prev_block);
  prev_prev_inuse = HEADER_PREVINUSE(DEREF(prev_block));
  DEREF(prev_block) =
      PACK_SIZE(prev_size + current_size, inuse, prev_prev_inuse);
  DEREF_FROM_NTH(prev_block, prev_size + current_size - WORDSIZE) =
      prev_size + current_size;
  return prev_block;
}

/*
 * should_split - check if `block` should be split, if only `size` bytes are
 * needed.
 * Returns true if a new block with minimal size of 4 words can be created.
 */
static bool should_split(word_t *block, size_t size) {
  size_t block_size = HEADER_SIZE(block[0]);
  return block_size >= size + 16 * WORDSIZE;
}

/*
 * handle_exceptional_size - handle exceptional size.
 * Returns the size after handling.
 */
static size_t handle_exceptional_size(size_t size) {
  if (size == 448)
    size = 512;
  else if (size == 112)
    size = 128;
  return size;
}

/*
 * mm_check - Check the heap for consistency.
 * Returns 1 if the heap is consistent, 0 otherwise.
 *
 * Tests for free list consistency, inuse/prev_inuse bit consistency, and
 * coalescing.
 */
int mm_check(void) {
  struct header *current;
  word_t *current_block;
  bool found;

  /* Check free list consistency. */
  for (current = freelist_head; current != NULL; current = current->next) {
    if (HEADER_INUSE(current->size_with_flags)) {
      fprintf(stderr, "%p: Free list contains an allocated block.\n", current);
      return 0;
    }
  }

  current_block = (word_t *)mem_heap_lo() + 3;
  while (current_block < (word_t *)mem_heap_hi() - 1) {
    /* Check if the block has nonzero size. */
    if (HEADER_SIZE(DEREF(current_block)) == 0) {
      fprintf(stderr, "%p: Block with size zero\n", current_block);
      return 0;
    }

    /* Check if header and footer size are consistent. */
    if (!HEADER_INUSE(DEREF(current_block)) &&
        HEADER_SIZE(DEREF(current_block)) != DEREF(FOOTER(current_block))) {
      fprintf(stderr,
              "%p: Header and footer size of free block is inconsistent.\n",
              current_block);
      return 0;
    }

    /* Check if coalescing is correct. */
    if (!HEADER_INUSE(DEREF(current_block)) &&
        !HEADER_INUSE(DEREF(NEXT_BLOCK(current_block)))) {
      fprintf(stderr, "%p: Two consecutive free blocks.\n", current_block);
      return 0;
    }

    /* Check if inuse and prev_inuse bits are consistent. */
    if (HEADER_INUSE(DEREF(current_block)) !=
        HEADER_PREVINUSE(DEREF(NEXT_BLOCK(current_block)))) {
      fprintf(stderr,
              "%p: The inuse bit of current block and prev_inuse bit of the "
              "next block is inconsistent.\n",
              current_block);
      return 0;
    }

    /* Check if the block is in the free list. */
    if (!HEADER_INUSE(DEREF(current_block))) {
      found = false;
      for (current = freelist_head; current != NULL; current = current->next) {
        if (current == (struct header *)current_block) {
          found = true;
          break;
        }
      }
      if (!found) {
        fprintf(stderr, "%p: Block not in free list.\n", current_block);
        return 0;
      }
    }

    current_block = NEXT_BLOCK(current_block);
  }

  return 1;
}

/*
 * mm_init - initialize the malloc package.
 * Prologue and epilogue blocks are created. The initial free block is of
 * CHUNKSIZE.
 */
int mm_init(void) {
  word_t *prologue_block, *init_block;
  struct header *init_block_header;

  prologue_block = mem_sbrk(4 * WORDSIZE);
  if (prologue_block == (void *)-1)
    return -1;

  prologue_block[0] = 0;
  prologue_block[1] = PACK_SIZE(8, 1, 0);
  prologue_block[2] = 8;
  prologue_block[3] = PACK_SIZE(0, 1, 1);

  init_block = expand_heap(CHUNKSIZE / WORDSIZE);
  if (init_block == NULL)
    return -1;

  init_block_header = (struct header *)init_block;
  freelist_head = NULL;
  freelist_tail = NULL;
  list_insert(init_block_header);
  return 0;
}

/*
 * mm_malloc - Allocate a block.
 * Always allocate a block whose size is a multiple of the alignment. Looks for
 * a free block in the free list, using best-fit strategy. If no free block is
 * found, allocate a new block using `expand_heap`.
 */
void *mm_malloc(size_t size) {
  size = handle_exceptional_size(size);

  int newsize = ALIGN(size + WORDSIZE);
  word_t *block = find_best_fit(newsize), *new_block, *next_block;

  if (size == 0)
    return NULL;

  if (block == NULL) {
    block = expand_heap(MAX(newsize, CHUNKSIZE) / WORDSIZE);
    if (block == NULL)
      return NULL;
    block = coalesce_block(block);
    list_insert((struct header *)block);
    block = find_best_fit(newsize);
  }

  list_remove((struct header *)block);
  if (should_split(block, newsize)) {
    new_block = split_block(block, newsize);
    HEADER_PREVINUSE_SET(DEREF(new_block));
    new_block = coalesce_block(new_block);
    list_insert((struct header *)new_block);
  } else {
    next_block = NEXT_BLOCK(block);
    HEADER_PREVINUSE_SET(DEREF(next_block));
  }
  HEADER_INUSE_SET(DEREF(block));

  return (void *)(block + 1);
}

/*
 * mm_free - Free a block.
 * Coalesce the block and insert to free list.
 */
void mm_free(void *ptr) {
  word_t *block = (word_t *)ptr - 1, *next_block;

  HEADER_INUSE_CLEAR(DEREF(block));
  *FOOTER(block) = HEADER_SIZE(DEREF(block));

  next_block = NEXT_BLOCK(block);
  HEADER_PREVINUSE_CLEAR(DEREF(next_block));

  block = coalesce_block(block);
  list_insert((struct header *)block);
}

/*
 * mm_realloc - Reallocate a block.
 * After special-case handling, if the new size is smaller than the old size,
 * shrink the block and split if needed. If the new size is larger than the old
 * size, check if the block can be expanded. If not, allocate a new block and
 * copy the contents of the old block.
 */
void *mm_realloc(void *ptr, size_t size) {
  int prev_inuse, prev_prev_inuse;
  word_t *block = PAYLOAD_HEADER(ptr), *next_block, *new_block, *prev_block;
  size_t oldsize = HEADER_SIZE(DEREF(block)), newsize = ALIGN(size + WORDSIZE),
         next_size, prev_size;
  void *newptr;

  if (size == 0) {
    mm_free(ptr);
    return NULL;
  } else if (ptr == NULL) {
    return mm_malloc(size);
  }

  prev_inuse = HEADER_PREVINUSE(DEREF(block));
  if (oldsize >= newsize) {
    if (should_split(block, newsize)) {
      next_block = NEXT_BLOCK(block);
      HEADER_PREVINUSE_CLEAR(DEREF(next_block));

      DEREF(block) = PACK_SIZE(newsize, 1, prev_inuse);
      DEREF_FROM_NTH(block, newsize) = PACK_SIZE(oldsize - newsize, 0, 1);
      DEREF_FROM_NTH(block, oldsize - WORDSIZE) = oldsize - newsize;
      new_block = NEXT_BLOCK(block);
      new_block = coalesce_block(new_block);
      list_insert((struct header *)new_block);
    }
    return ptr;
  } else {
    next_block = NEXT_BLOCK(block);
    next_size = HEADER_SIZE(DEREF(next_block));
    if (!HEADER_INUSE(DEREF(next_block)) && oldsize + next_size >= newsize) {
      list_remove((struct header *)next_block);
      DEREF(block) = PACK_SIZE(oldsize + next_size, 1, prev_inuse);
      if (should_split(block, newsize)) {
        DEREF(block) = PACK_SIZE(newsize, 1, prev_inuse);
        new_block = &DEREF_FROM_NTH(block, newsize);
        DEREF(new_block) = PACK_SIZE(oldsize + next_size - newsize, 0, 1);
        DEREF(FOOTER(new_block)) = oldsize + next_size - newsize;
        new_block = coalesce_block(new_block);
        list_insert((struct header *)new_block);
      } else
        HEADER_PREVINUSE_SET(DEREF(NEXT_BLOCK(block)));
      return ptr;
    } else if (!HEADER_PREVINUSE(DEREF(block)) &&
               oldsize + HEADER_SIZE(DEREF(PREV_BLOCK(block))) >= newsize) {
      prev_size = HEADER_SIZE(DEREF(PREV_BLOCK(block)));
      prev_block = PREV_BLOCK(block);
      prev_prev_inuse = HEADER_PREVINUSE(DEREF(prev_block));
      list_remove((struct header *)prev_block);
      DEREF(prev_block) = PACK_SIZE(prev_size + oldsize, 1, prev_prev_inuse);
      memmove(prev_block + 1, block + 1, oldsize - WORDSIZE);
      if (should_split(prev_block, newsize)) {
        DEREF(prev_block) = PACK_SIZE(newsize, 1, prev_prev_inuse);
        new_block = &DEREF_FROM_NTH(prev_block, newsize);
        DEREF(new_block) = PACK_SIZE(prev_size + oldsize - newsize, 0, 1);
        DEREF(FOOTER(new_block)) = prev_size + oldsize - newsize;
        new_block = coalesce_block(new_block);
        list_insert((struct header *)new_block);
        HEADER_PREVINUSE_CLEAR(DEREF(NEXT_BLOCK(new_block)));
      }
      return prev_block + 1;
    } else if (!HEADER_INUSE(DEREF(next_block)) &&
               !HEADER_PREVINUSE(DEREF(block)) &&
               oldsize + HEADER_SIZE(DEREF(PREV_BLOCK(block))) + next_size >=
                   newsize) {
      prev_size = HEADER_SIZE(DEREF(PREV_BLOCK(block)));
      prev_block = PREV_BLOCK(block);
      prev_prev_inuse = HEADER_PREVINUSE(DEREF(prev_block));
      list_remove((struct header *)prev_block);
      list_remove((struct header *)next_block);
      DEREF(prev_block) =
          PACK_SIZE(prev_size + oldsize + next_size, 1, prev_prev_inuse);
      memmove(prev_block + 1, block + 1, oldsize - WORDSIZE);
      if (should_split(prev_block, newsize)) {
        DEREF(prev_block) = PACK_SIZE(newsize, 1, prev_prev_inuse);
        new_block = &DEREF_FROM_NTH(prev_block, newsize);
        DEREF(new_block) =
            PACK_SIZE(prev_size + oldsize + next_size - newsize, 0, 1);
        DEREF(FOOTER(new_block)) = prev_size + oldsize + next_size - newsize;
        new_block = coalesce_block(new_block);
        list_insert((struct header *)new_block);
        HEADER_PREVINUSE_CLEAR(DEREF(NEXT_BLOCK(new_block)));
      } else
        HEADER_PREVINUSE_SET(DEREF(NEXT_BLOCK(prev_block)));
      return prev_block + 1;
    } else {
      newptr = mm_malloc(size);
      if (newptr == NULL)
        return NULL;
      memcpy(newptr, ptr, oldsize - WORDSIZE);
      mm_free(ptr);
      return newptr;
    }
  }
}
