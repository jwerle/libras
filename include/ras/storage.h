#ifndef RAS_STORAGE_H
#define RAS_STORAGE_H

#include "emitter.h"
#include "request.h"
#include "platform.h"

// Forward declarations
struct ras_emitter_s;
struct ras_request_s;
struct ras_storage_s;
struct ras_storage_stats_s;
struct ras_storage_options_s;

/**
 * The maximum queued requests.
 */
#ifndef RAS_STORAGE_MAX_REQUEST_QUEUE
#define RAS_STORAGE_MAX_REQUEST_QUEUE 512
#endif

/**
 * The `ras_storage_request_callback_t` callback represents the user
 * callback for the random access work request to be done.
 */
typedef void (ras_storage_request_callback_t)(struct ras_request_s *request);

/**
 * The `ras_storage_open_callback_t` callback represents the user callback
 * for a random access open request.
 */
typedef void (ras_storage_open_callback_t)(
  struct ras_storage_s *storage,
  int err);

/**
 * The `ras_storage_open_read_only_callback_t` callback represents the user callback
 * for a random access open_read_only request.
 */
typedef void (ras_storage_open_read_only_callback_t)(
  struct ras_storage_s *storage,
  int err);

/**
 * The `ras_storage_read_callback_t` callback represents the user callback
 * for a random access read request.
 */
typedef void (ras_storage_read_callback_t)(
  struct ras_storage_s *storage,
  int err,
  void *buffer,
  unsigned long int size);

/**
 * The `ras_storage_write_callback_t` callback represents the user callback
 * for a random access write request.
 */
typedef void (ras_storage_write_callback_t)(
  struct ras_storage_s *storage,
  int err);

/**
 * The `ras_storage_delete_callback_t` callback represents the user callback
 * for a random access delete request.
 */
typedef void (ras_storage_delete_callback_t)(
  struct ras_storage_s *storage,
  int err);

/**
 * The `ras_storage_stat_callback_t` callback represents the user callback
 * for a random access stat request.
 */
typedef void (ras_storage_stat_callback_t)(
  struct ras_storage_s *storage,
  int err,
  struct ras_storage_stats_s *stats);

/**
 * The `ras_storage_close_callback_t` callback represents the user callback
 * for a random access close request.
 */
typedef void (ras_storage_close_callback_t)(
  struct ras_storage_s *storage,
  int err);

/**
 * The `ras_storage_destroy_callback_t` callback represents the user callback
 * for a random access destroy request.
 */
typedef void (ras_storage_destroy_callback_t)(
  struct ras_storage_s *storage,
  int err);

/**
 * Fields for `struct ras_storage_options_s` that can be used for
 * extending structures that ensure correct memory layout.
 *
 * layout= [
 *   open_read_only=0, open=1, read=2, write=3,
 *   del=4, stat=5, close=6, destroy=7, data=8,
 * ]
 */
#define RAS_STORAGE_OPTIONS_FIELDS                \
  ras_storage_request_callback_t *open_read_only; \
  ras_storage_request_callback_t *open;           \
  ras_storage_request_callback_t *read;           \
  ras_storage_request_callback_t *write;          \
  ras_storage_request_callback_t *del;            \
  ras_storage_request_callback_t *stat;           \
  ras_storage_request_callback_t *close;          \
  ras_storage_request_callback_t *destroy;        \
  void *data;

/**
 * Represents the initial configurable state for a random access storage
 * context.
 */
struct ras_storage_options_s {
  RAS_STORAGE_OPTIONS_FIELDS
};

/**
 * Fields for `struct ras_storage_s` that can be used for
 * extending structures that ensure correct memory layout.
 */
#define RAS_STORAGE_FIELDS                                     \
  unsigned int alloc:1;                                        \
  unsigned int opened:1;                                       \
  unsigned int closed:1;                                       \
  unsigned int queued;                                         \
  unsigned int pending;                                        \
  unsigned int readable:1;                                     \
  unsigned int statable:1;                                     \
  unsigned int writable:1;                                     \
  unsigned int deletable:1;                                    \
  unsigned int destroyed:1;                                    \
  unsigned int needs_open:1;                                   \
  unsigned int prefer_read_only:1;                             \
  struct ras_emitter_s emitter;                                \
  struct ras_request_s last_request;                           \
  struct ras_request_s *queue[RAS_STORAGE_MAX_REQUEST_QUEUE];  \
  struct ras_storage_options_s options;                        \
  void *data;                                                  \

/**
 * Represents the state for a random access storage context.
 */
struct ras_storage_s {
  RAS_STORAGE_FIELDS
};

/**
 * Fields for `struct ras_storage_stats_s` that can be used for
 * extending structures that ensure correct memory layout.
 */
#define RAS_STORAGE_STATS_FIELDS \
  unsigned long int size;        \
  void *extended;

/**
 * Represents the state for random access storage stats.
 */
struct ras_storage_stats_s {
  RAS_STORAGE_STATS_FIELDS
};

/**
 * Allocates a pointer to 'struct ras_storage_s'.
 * Calls 'ras_alloc()' internally and will return 'NULL'
 * on allocation errors.
 */
RAS_EXPORT struct ras_storage_s *
ras_storage_alloc();

/**
 * Initializes a pointer to `struct ras_storage_s` with options from
 * `struct ras_storage_options_s`. Returns `0` on success, otherwise an
 * error code found in `errno.h` with its sign flipped and `errno` set.
 *
 * Possible Error Codes
 *   * `EFAULT`: The 'struct ras_storage_s *storage' is `NULL`
 */
RAS_EXPORT int
ras_storage_init(
  struct ras_storage_s *storage,
  struct ras_storage_options_s options);

/**
 * Allocates and initializes a pointer to `struct ras_storage_s`. Returns
 * `NULL` on error and `errno` is set to an error code found in `errno.h`.
 */
RAS_EXPORT struct ras_storage_s *
ras_storage_new(struct ras_storage_options_s options);

/**
 * Frees a pointer to `struct ras_storage_s`.
 */
RAS_EXPORT void
ras_storage_free(struct ras_storage_s *storage);

/**
 * Opens the storage interface. The storage interface must be initialized
 * with an `open()` operation in `struct ras_storage_options_s` given to
 * `ras_storage_new()` or `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_open(
  struct ras_storage_s *storage,
  ras_storage_open_callback_t *callback);

RAS_EXPORT int
ras_storage_open_shared(
  struct ras_storage_s *storage,
  ras_storage_open_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Reads a buffer from the storage interface. The storage interface must be
 * initialized with a `read()` operation in `struct ras_storage_options_s`
 * given to `ras_storage_new()` or `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_read(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_read_callback_t *callback);

RAS_EXPORT int
ras_storage_read_shared(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_read_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Writes a buffer to the storage interface. The storage interface must be
 * initialized with a `write()` operation in `struct ras_storage_options_s`
 * given to `ras_storage_new()` or `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_write(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  const void *buffer,
  ras_storage_write_callback_t *callback);

RAS_EXPORT int
ras_storage_write_shared(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  const void *buffer,
  ras_storage_write_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Deletes a buffer regionfrom the storage interface. The storage interface
 * must be initialized with a `del()` operation in
 * `struct ras_storage_options_s` given to `ras_storage_new()` or
 * `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_delete(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_delete_callback_t *callback);

RAS_EXPORT int
ras_storage_delete_shared(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_delete_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Queries the storage interface for stats. The storage interface must be
 * initialized with a `stat()` operation `struct ras_storage_options_s`
 * given to `ras_storage_new()` or `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_stat(
  struct ras_storage_s *storage,
  ras_storage_stat_callback_t *callback);

RAS_EXPORT int
ras_storage_stat_shared(
  struct ras_storage_s *storage,
  ras_storage_stat_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Closes the storage interface. The storage interface must be initialized
 * with a `close()` operation in `struct ras_storage_options_s` given to
 * `ras_storage_new()` or `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_close(
  struct ras_storage_s *storage,
  ras_storage_close_callback_t *callback);

RAS_EXPORT int
ras_storage_close_shared(
  struct ras_storage_s *storage,
  ras_storage_close_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Destroys and frees a pointer to `struct ras_storage_s` to the storage
 * interface. To handle the destruction of more memory, the storage interface
 * must be initialized with a `destroy()` operation in
 * `struct ras_storage_options_s` given to `ras_storage_new()` or
 * `ras_storage_init()`.
 */
RAS_EXPORT int
ras_storage_destroy(
  struct ras_storage_s *storage,
  ras_storage_destroy_callback_t *callback);

RAS_EXPORT int
ras_storage_destroy_shared(
  struct ras_storage_s *storage,
  ras_storage_destroy_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared);

/**
 * Returns the head of the queue pointing to a `struct ras_request_s` type and
 * shifts the remaining elements over to the left by 1.
 */
RAS_EXPORT struct ras_request_s *
ras_storage_queue_shift(struct ras_storage_s *storage);

/**
 * Pushes a `struct ras_request_s` pointer on to the queue returning
 * the new queue length.
 */
RAS_EXPORT int
ras_storage_queue_push(
  struct ras_storage_s *storage,
  struct ras_request_s *request);

#endif
