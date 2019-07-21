#include "ras/allocator.h"
#include <stdlib.h>

#ifndef RAS_ALLOCATOR_ALLOC
#define RAS_ALLOCATOR_ALLOC 0
#endif

#ifndef RAS_ALLOCATOR_FREE
#define RAS_ALLOCATOR_FREE 0
#endif

static void *(*alloc)(unsigned long int) = RAS_ALLOCATOR_ALLOC;
static void (*dealloc)(void *) = RAS_ALLOCATOR_FREE;

static struct ras_allocator_stats_s stats = { 0 };

const struct ras_allocator_stats_s
ras_allocator_stats() {
  return (struct ras_allocator_stats_s) {
    .alloc = stats.alloc,
    .free = stats.free
  };
}

int
ras_allocator_alloc_count() {
  return stats.alloc;
}

int
ras_allocator_free_count() {
  return stats.free;
}

void
ras_allocator_set(void *(*allocator)(unsigned long int)) {
  alloc = allocator;
}

void
ras_deallocator_set(void (*deallocator)(void *)) {
  dealloc = deallocator;
}

void *
ras_alloc(unsigned long int size) {
  if (0 == size) {
    return 0;
  } else if (0 != alloc) {
    (void) stats.alloc++;
    return alloc(size);
  } else {
    (void) stats.alloc++;
    return malloc(size);
  }
}

void
ras_free(void *ptr) {
  if (0 == ptr) {
    return;
  } else if (0 != dealloc) {
    (void) stats.free++;
    dealloc(ptr);
  } else {
    (void) stats.free++;
    free(ptr);
  }
}
