#include "cachelab.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum access_type { ACCESS_LOAD, ACCESS_STORE, ACCESS_MODIFY };
struct access_info {
  enum access_type type;
  uint64_t addr;
  int size;
};

bool next_access_info(struct access_info *, FILE *);

int main()
{
    printSummary(0, 0, 0);
    return 0;
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
