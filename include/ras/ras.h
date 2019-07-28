#ifndef RAS_H
#define RAS_H

#include "allocator.h"
#include "emitter.h"
#include "platform.h"
#include "request.h"
#include "storage.h"
#include "version.h"

/**
 * The `ras_storage_t` (`struct ras_storage_s`) type represents
 * the storage context for a random access storage interface.
 */
typedef struct ras_storage_s ras_storage_t;

/**
 * The `ras_storage_options_t` (`struct ras_storage_options_s`) type represents
 * the initialization options for the `ras_storage_t` type used with the
 * `ras_storge_new()` and `ras_storge_init()` functions.
 */
typedef struct ras_storage_options_s ras_storage_options_t;

/**
 * The `ras_request_t` (`struct ras_request_s`) type represents a request
 * operation that may be initiated by one of the `ras_storage_*()` functions
 * that *open*, *close*, *read*, *write, *stat*, and *destroy* on random access
 * storage interface implementations. A request contains the information needed
 * to complete a request operation and call back to the caller.
 */
typedef struct ras_request_s ras_request_t;

/**
 * The `ras_storage_stats_t` (`struct ras_storage_stats_s`) type represents a
 * structure containing stats about the random access storage interface, such
 * as `size`.
 */
typedef struct ras_storage_stats_s ras_storage_stats_t;

/**
 * The `ras_allocator_stats_t` (`struct ras_allocator_stats_s`) type represents
 * a structure of counters for the number of times `ras_alloc()` and
 * `ras_free()` have been called.
 */
typedef struct ras_allocator_stats_s ras_allocator_stats_t;

/**
 * The `ras_request_type_t` (`enum ras_request_type`) type is an enumeration
 * of the possible request type the implementation can process.
 */
typedef enum ras_request_type ras_request_type_t;

/**
 */
typedef struct ras_emitter_s ras_emitter_t;

/**
 */
typedef struct ras_emitter_listener_s ras_emitter_listener_t;

/**
 */
typedef enum ras_event ras_event_t;

#endif
