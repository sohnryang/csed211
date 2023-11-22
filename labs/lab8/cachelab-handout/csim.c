/* 20220323 Ryang Sohn */
#include "cachelab.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct cache_line {
  bool valid;
  uint64_t tag;
  int last_accesed;
};

struct cache_set {
  bool valid;
  int line_count;
  struct cache_line *lines;
};

struct cache_store {
  int set_bits;
  int lines_per_set;
  int block_bits;
  struct cache_set *sets;
};

struct simulation_result {
  int hits;
  int misses;
  int evictions;
};

enum access_type { ACCESS_LOAD, ACCESS_STORE, ACCESS_MODIFY };
struct access_info {
  enum access_type type;
  uint64_t addr;
  int size;
};

void print_help(void);
bool next_access_info(struct access_info *, FILE *);

void cache_store_init(struct cache_store *, int, int, int);
void cache_set_init(struct cache_set *, int);

uint64_t addr_block(struct cache_store *, uint64_t);
uint64_t addr_set(struct cache_store *, uint64_t);
uint64_t addr_tag(struct cache_store *, uint64_t);

struct cache_line *find_matching_line(struct cache_set *, uint64_t);
struct cache_line *find_empty_line(struct cache_set *);
struct cache_line *find_evicted_line(struct cache_set *);
void simulate_access(struct cache_store *, struct simulation_result *,
                     struct access_info *, int, bool);

#ifndef UNIT_TESTING
int main(int argc, char *argv[]) {
  bool help_flag, verbose_flag;
  char *set_bits_str, *lines_per_set_str, *block_bits_str, *trace_filename;
  int set_bits, lines_per_set, block_bits, opt_ch, ticks;
  FILE *trace_file;
  struct cache_store store;
  struct access_info info;
  struct simulation_result result = {0, 0, 0};

  opterr = 0;
  help_flag = false;
  verbose_flag = false;
  while ((opt_ch = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt_ch) {
    case 'h':
      help_flag = true;
      break;
    case 'v':
      verbose_flag = true;
      break;
    case 's':
      set_bits_str = optarg;
      break;
    case 'E':
      lines_per_set_str = optarg;
      break;
    case 'b':
      block_bits_str = optarg;
      break;
    case 't':
      trace_filename = optarg;
      break;
    case '?':
      if (optopt == 's' || optopt == 'E' || optopt == 'b' || optopt == 't')
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      else
        fprintf(stderr, "Unknown option -%c.\n", optopt);
      return 1;
    default:
      abort();
    }
  }

  if (help_flag) {
    print_help();
    return 0;
  }

  set_bits = strtol(set_bits_str, NULL, 10);
  if (set_bits <= 0) {
    fprintf(stderr, "Invalid set bit count: %s\n", set_bits_str);
    return 1;
  }

  lines_per_set = strtol(lines_per_set_str, NULL, 10);
  if (lines_per_set <= 0) {
    fprintf(stderr, "Invalid lines per set count: %s\n", lines_per_set_str);
    return 1;
  }

  block_bits = strtol(block_bits_str, NULL, 10);
  if (block_bits <= 0) {
    fprintf(stderr, "Invalid block bit count: %s\n", block_bits_str);
    return 1;
  }

  trace_file = fopen(trace_filename, "r");
  if (trace_file == NULL) {
    fprintf(stderr, "Could not open file: %s\n", trace_filename);
    return 1;
  }

  ticks = 0;
  cache_store_init(&store, set_bits, lines_per_set, block_bits);
  while (next_access_info(&info, trace_file)) {
    simulate_access(&store, &result, &info, ticks, verbose_flag);
    ticks++;
  }
  printSummary(result.hits, result.misses, result.evictions);

  return 0;
}
#endif

void print_help(void) {
  printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
         "Options:\n"
         "  -h\t\tPrint this help message.\n"
         "  -v\t\tOptional verbose flag.\n"
         "  -s <num>\tNumber of set index bits.\n"
         "  -E <num>\tNumber of lines per set.\n"
         "  -b <num>\tNumber of block offset bits.\n"
         "  -t <file>\tTrace file.\n\n"
         "Examples:\n"
         "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
         "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void cache_store_init(struct cache_store *store, int set_bits,
                      int lines_per_set, int block_bits) {
  store->set_bits = set_bits;
  store->lines_per_set = lines_per_set;
  store->block_bits = block_bits;
  store->sets = calloc(1 << set_bits, sizeof(struct cache_set));
}

void cache_set_init(struct cache_set *set, int lines_per_set) {
  set->valid = true;
  set->line_count = lines_per_set;
  set->lines = calloc(lines_per_set, sizeof(struct cache_line));
}

bool next_access_info(struct access_info *info, FILE *file) {
  char buf[256], *res, type_ch, addr_str[256], size_str[256];
  enum access_type type;
  int i, size;
  uint64_t addr;

  do {
    res = fgets(buf, 256, file);
    if (res == NULL)
      return false;
  } while (buf[0] == 'I');

  type_ch = buf[1];
  switch (type_ch) {
  case 'L':
    type = ACCESS_LOAD;
    break;
  case 'S':
    type = ACCESS_STORE;
    break;
  case 'M':
    type = ACCESS_MODIFY;
    break;
  default:
    return false;
  }

  for (i = 3; buf[i] != ','; i++)
    addr_str[i - 3] = buf[i];
  addr_str[i - 3] = '\0';
  strncpy(size_str, buf + i + 1, sizeof(size_str) - 1);
  size_str[255] = '\0';

  addr = strtoull(addr_str, NULL, 16);

  size = strtol(size_str, NULL, 10);
  if (size == 0)
    return false;

  info->type = type;
  info->addr = addr;
  info->size = size;
  return true;
}

uint64_t addr_block(struct cache_store *store, uint64_t addr) {
  return addr & ((1 << store->block_bits) - 1);
}

uint64_t addr_set(struct cache_store *store, uint64_t addr) {
  return (addr >> store->block_bits) & ((1 << store->set_bits) - 1);
}

uint64_t addr_tag(struct cache_store *store, uint64_t addr) {
  return addr >> (store->set_bits + store->block_bits);
}

struct cache_line *find_matching_line(struct cache_set *cache_set,
                                      uint64_t tag) {
  for (int i = 0; i < cache_set->line_count; i++)
    if (cache_set->lines[i].valid && cache_set->lines[i].tag == tag)
      return &cache_set->lines[i];
  return NULL;
}

struct cache_line *find_empty_line(struct cache_set *cache_set) {
  for (int i = 0; i < cache_set->line_count; i++)
    if (!cache_set->lines[i].valid)
      return &cache_set->lines[i];
  return NULL;
}

struct cache_line *find_evicted_line(struct cache_set *cache_set) {
  struct cache_line *evicted;

  evicted = cache_set->lines;
  for (int i = 1; i < cache_set->line_count; i++)
    evicted = cache_set->lines[i].last_accesed < evicted->last_accesed
                  ? &cache_set->lines[i]
                  : evicted;

  return evicted;
}

void simulate_access(struct cache_store *store,
                     struct simulation_result *result, struct access_info *info,
                     int ticks, bool verbose) {
  uint64_t set_idx, tag;
  bool missed, evicted;
  struct cache_set *set;
  struct cache_line *line;

  set_idx = addr_set(store, info->addr);
  set = &store->sets[set_idx];
  if (!set->valid)
    cache_set_init(set, store->lines_per_set);

  tag = addr_tag(store, info->addr);
  line = find_matching_line(set, tag);
  if (line == NULL) {
    missed = true;
    line = find_empty_line(set);
    if (line == NULL) {
      evicted = true;
      line = find_evicted_line(set);
    }
  } else
    missed = false;

  line->valid = true;
  line->tag = tag;
  line->last_accesed = ticks;

  result->misses += missed;
  result->hits += !missed;
  result->hits += info->type == ACCESS_MODIFY;
  result->evictions += evicted;

  if (verbose) {
    switch (info->type) {
    case ACCESS_LOAD:
      printf("L %lx,%d %s%s\n", info->addr, info->size,
             missed ? "miss " : "hit ", evicted ? "eviction " : "");
      break;
    case ACCESS_STORE:
      printf("S %lx,%d %s%s\n", info->addr, info->size,
             missed ? "miss " : "hit ", evicted ? "eviction " : "");
      break;
    case ACCESS_MODIFY:
      printf("M %lx,%d %s%shit \n", info->addr, info->size,
             missed ? "miss " : "hit ", evicted ? "eviction " : "");
      break;
    }
  }
}
