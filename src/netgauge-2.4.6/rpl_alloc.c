#include <stdlib.h>

void * rpl_malloc (size_t n) {
  if (n == 0)
    n = 1;
  return malloc (n);
}

void * rpl_realloc (void *n, size_t m) {
  if (m == 0)
    m = 1;
  return realloc (n, m);
}
