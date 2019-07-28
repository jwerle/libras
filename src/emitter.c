#include "ras/emitter.h"
#include "require.h"

int
ras_emitter_init(struct ras_emitter_s *emitter) {
  require(emitter, EFAULT);
  emitter->length = 0;
  for (int i = 0; i < RAS_EMITTER_MAX_LISTENERS; ++i) {
    emitter->listeners[i] = (struct ras_emitter_listener_s) { 0 };
  }
  return 0;
}

int
ras_emitter_clear(struct ras_emitter_s *emitter) {
  return 0;
}

int
ras_emitter_on(
  struct ras_emitter_s *emitter,
  struct ras_emitter_listener_s listener
) {
  require(emitter, EFAULT);
  require(emitter->length < RAS_EMITTER_MAX_LISTENERS, ENOMEM);
  int upsert = 0;

  listener.ttl = -1;
  for (int i = 0; i < RAS_EMITTER_MAX_LISTENERS; ++i) {
    if (0 == emitter->listeners[i].event) {
      upsert = i;
      emitter->listeners[i] = listener;
      break;
    }
  }

  if (0 == upsert) {
    emitter->listeners[emitter->length++] = listener;
  } else {
    emitter->length = upsert + 1;
  }

  return emitter->length;
}

int
ras_emitter_once(
  struct ras_emitter_s *emitter,
  struct ras_emitter_listener_s listener
) {
  require(emitter, EFAULT);
  require(emitter->length < RAS_EMITTER_MAX_LISTENERS, ENOMEM);
  int upsert = 0;

  listener.ttl = 1;

  for (int i = 0; i < RAS_EMITTER_MAX_LISTENERS; ++i) {
    if (0 == emitter->listeners[i].event) {
      upsert = i;
      emitter->listeners[i] = listener;
      break;
    }
  }

  if (0 == upsert) {
    emitter->listeners[emitter->length++] = listener;
  } else {
    emitter->length = upsert + 1;
  }

  return emitter->length;
}

int
ras_emitter_off(
  struct ras_emitter_s *emitter,
  struct ras_emitter_listener_s listener
) {
  require(emitter, EFAULT);

  int end = RAS_EMITTER_MAX_LISTENERS;

  // remove all
  if (0 == listener.callback && listener.event > 0) {
    for (int i = 0; i < RAS_EMITTER_MAX_LISTENERS; ++i) {
      if (listener.event == emitter->listeners[i].event) {
        emitter->listeners[i] = (struct ras_emitter_listener_s) { 0 };
      }
    }
  } else if (listener.callback) {
    for (int i = 0; i < RAS_EMITTER_MAX_LISTENERS; ++i) {
      if (listener.callback == emitter->listeners[i].callback) {
        emitter->listeners[i] = (struct ras_emitter_listener_s) { 0 };
      }
    }
  }

  while (
    end >= 0 &&
    0 == emitter->listeners[end - 1].event &&
    0 == emitter->listeners[end - 1].callback
  ) {
    (void) end--;
  }

  emitter->length = end;
  return emitter->length;
}

int
ras_emitter_emit(
  struct ras_emitter_s *emitter,
  unsigned long int event,
  void *value
) {
  int expired = 0;
  int events = 0;

  for (int i = 0; i < emitter->length; ++i) {
    struct ras_emitter_listener_s *listener = &emitter->listeners[i];
    if (event == listener->event && 0 != listener->ttl) {
      if (0 != listener->callback) {
        (void) events++;
        listener->callback(value, listener->data);
      }

      if (-1 != listener->ttl) {
        (void) listener->ttl--;
      }

      if (0 == listener->ttl) {
        (void) expired++;
      }
    }
  }

  while (expired-- > 0) {
    for (int i = 0; i < emitter->length; ++i) {
      if (0 == emitter->listeners[i].ttl) {
        ras_emitter_off(emitter, emitter->listeners[i]);
      }
    }
  }

  return events;
}
