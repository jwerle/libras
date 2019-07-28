#ifndef RAS_EMITTER_H
#define RAS_EMITTER_H

#include "platform.h"

// Forward declarations
struct ras_emitter_s;
struct ras_emitter_listener_s;

/**
 */
#ifndef RAS_EMITTER_MAX_LISTENERS
#define RAS_EMITTER_MAX_LISTENERS 64
#endif

/**
 */
typedef void (ras_emitter_listener_callback_t)(void *value, void *data);

/**
 */
struct ras_emitter_listener_s {
  unsigned long int event;
  ras_emitter_listener_callback_t *callback;
  void *data;
  int ttl;
};

/**
 */
#define RAS_EMITTER_FIELDS                                            \
  struct ras_emitter_listener_s listeners[RAS_EMITTER_MAX_LISTENERS]; \
  unsigned long int length;                                           \

struct ras_emitter_s {
  RAS_EMITTER_FIELDS
};

/**
 */
enum ras_event {
  RAS_EVENT_ERROR = 0xff - 1,
  RAS_EVENT_READ = 0xff + 0,
  RAS_EVENT_WRITE = 0xff + 1,
  RAS_EVENT_DELETE = 0xff + 2,
  RAS_EVENT_STAT = 0xff + 3,
  RAS_EVENT_OPEN = 0xff + 4,
  RAS_EVENT_CLOSE = 0xff + 5,
  RAS_EVENT_DESTROY = 0xff + 6,
  RAS_STORAGE_EVENT_NONE = RAS_MAX_ENUM
};

/**
 */
RAS_EXPORT int
ras_emitter_init(struct ras_emitter_s *emitter);

/**
 */
RAS_EXPORT int
ras_emitter_clear(struct ras_emitter_s *emitter);

/**
 */
RAS_EXPORT int
ras_emitter_on(
  struct ras_emitter_s *emitter,
  struct ras_emitter_listener_s listener);

/**
 */
RAS_EXPORT int
ras_emitter_once(
  struct ras_emitter_s *emitter,
  struct ras_emitter_listener_s listener);

/**
 */
RAS_EXPORT int
ras_emitter_off(
  struct ras_emitter_s *emitter,
  struct ras_emitter_listener_s listener);

/**
 */
RAS_EXPORT int
ras_emitter_emit(
  struct ras_emitter_s *emitter,
  unsigned long int event,
  void *value);

#endif
