#ifndef RAS_REQUEST_H
#define RAS_REQUEST_H

#include "platform.h"

// Forward declarations
struct ras_request_s;
struct ras_storage_s;
struct ras_request_options_s;

/**
 * The `ras_request_callback_t` callback represents the user
 * callback for the random access work request to be done.
 */
typedef int (ras_request_callback_t)(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size);

/**
 * Random access storage request types.
 */
enum ras_request_type {
  RAS_REQUEST_READ = 0,
  RAS_REQUEST_WRITE = 1,
  RAS_REQUEST_DELETE = 2,
  RAS_REQUEST_STAT = 3,
  RAS_REQUEST_OPEN = 4,
  RAS_REQUEST_CLOSE = 5,
  RAS_REQUEST_DESTROY = 6,
  RAS_REQUEST_NONE = RAS_MAX_ENUM
};

/**
 * Fields for `struct ras_request_options_s` that can be used for
 * extending structures that ensure correct memory layout.
 */
#define RAS_REQUEST_OPTIONS_FIELDS  \
  enum ras_request_type type;       \
  struct ras_storage_s *storage;    \
  ras_request_callback_t *before;   \
  ras_request_callback_t *after;    \
  ras_request_callback_t *emit;     \
  ras_request_callback_t *hook;     \
  unsigned long int offset;         \
  unsigned long int size;           \
  void *callback;                   \
  void *shared;                     \
  void *data;

/**
 * Represents the initial configurable state for a random access storage
 * opertion request context.
 */
struct ras_request_options_s {
  RAS_REQUEST_OPTIONS_FIELDS
};

/**
 * Fields for `struct ras_request_s` that can be used for
 * extending structures that ensure correct memory layout.
 */
#define RAS_REQUEST_FIELDS          \
  unsigned int alloc:1;             \
  unsigned int id;                  \
  int err;                          \
  unsigned long int offset;         \
  unsigned long int size;           \
  unsigned int pending:1;           \
  unsigned int destroyed:1;         \
  enum ras_request_type type;       \
  struct ras_storage_s *storage;    \
  ras_request_callback_t *before;   \
  ras_request_callback_t *callback; \
  ras_request_callback_t *after;    \
  ras_request_callback_t *emit;     \
  ras_request_callback_t *hook;     \
  void *shared;                     \
  void *data;                       \
  void *done;

/**
 * Represents the state for a random access storage operation context.
 */
struct ras_request_s {
  RAS_REQUEST_FIELDS
};

/**
 * Allocates a pointer to 'struct ras_request_s'.
 * Calls 'ras_alloc()' internally and will return 'NULL'
 * on allocation errors.
 */
RAS_EXPORT struct ras_request_s *
ras_request_alloc();

/**
 * Initializes a pointer to `struct ras_request_s` with options from
 * `struct ras_request_options_s`. Returns `0` on success, otherwise an
 * error code found in `errno.h` with its sign flipped and `errno` set.
 *
 * Possible Error Codes
 *   * `EFAULT`: The 'struct ras_request_s *request' is `NULL`
 */
RAS_EXPORT int
ras_request_init(
  struct ras_request_s *request,
  const struct ras_request_options_s options);

/**
 * Allocates and initializes a pointer to `struct ras_request_s`. Returns
 * `NULL` on error and `errno` is set to an error code found in `errno.h`.
 */
RAS_EXPORT struct ras_request_s *
ras_request_new(const struct ras_request_options_s options);

/**
 * Frees a pointer to `struct ras_request_s`.
 */
RAS_EXPORT void
ras_request_free(struct ras_request_s *request);

/**
 * Runs the request for a random access operation.
 */
RAS_EXPORT int
ras_request_run(struct ras_request_s *request);

/**
 * Handles the callback from the random access operation
 * request function given to the implementation.
 */
RAS_EXPORT int
ras_request_callback(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size);

#endif
