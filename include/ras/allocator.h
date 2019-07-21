#ifndef RAS_ALLOCATOR_H
#define RAS_ALLOCATOR_H

#include "platform.h"

// Forward declarations
struct ras_allocator_stats_s;

/**
 * Allocator stats
 */
struct ras_allocator_stats_s {
  unsigned int alloc;
  unsigned int free;
};

/**
 * Returns allocator stats.
 */
RAS_EXPORT const struct ras_allocator_stats_s
ras_allocator_stats();

/**
 * Returns number of allocations returned by `ras_alloc()`.
 */
RAS_EXPORT int
ras_allocator_alloc_count();

/**
 * Returns number of deallocations by `ras_free()`.
 */
RAS_EXPORT int
ras_allocator_free_count();

/**
 * Set the allocator function used in the library.
 * `malloc()=`
 */
RAS_EXPORT void
ras_allocator_set(void *(*allocator)(unsigned long int));

/**
 * Set the deallocator function used in the library.
 * `free()=`
 */
RAS_EXPORT void
ras_deallocator_set(void (*allocator)(void *));

/**
 * The allocator function used in the library.
 * Defaults to `malloc()`.
 */
RAS_EXPORT void *
ras_alloc(unsigned long int);

/**
 * The deallocator function used in the library.
 * Defaults to `free()`.
 */
RAS_EXPORT void
ras_free(void *);

#endif
