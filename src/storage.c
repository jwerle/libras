#include "ras/allocator.h"
#include "ras/storage.h"
#include "ras/emitter.h"
#include "require.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static int
run_request(
  struct ras_storage_s *storage,
  struct ras_request_s *request
) {
  if (1 == storage->needs_open && 0 == storage->opened) {
    request->err = ras_storage_open(storage, 0);
  }

  if (storage->queued > 0) {
    ras_storage_queue_push(storage, request);
  } else {
    return ras_request_run(request);
  }

  return - request->err;
}

static int
queue_and_run(
  struct ras_storage_s *storage,
  struct ras_request_s *request
) {
  ras_storage_queue_push(storage, request);

  if (0 == storage->pending) {
    return -ras_request_run(request);
  } else {
    return - request->err;
  }

}

struct ras_storage_s *
ras_storage_alloc() {
  return ras_alloc(sizeof(struct ras_storage_s));
}

int
ras_storage_init(
  struct ras_storage_s *storage,
  struct ras_storage_options_s options
) {
  require(storage, EFAULT);
  require(memset(storage, 0, sizeof(struct ras_storage_s)), EFAULT);
  require(
    memcpy(&storage->options, &options, sizeof(struct ras_storage_options_s)),
    EFAULT);

  storage->prefer_read_only = 0 != options.open_read_only;
  storage->needs_open = 1;
  storage->deletable = 0 != options.del;
  storage->readable = 0 != options.read;
  storage->statable = 0 != options.stat;
  storage->writable = 0 != options.write;
  storage->data = options.data;
  return 0;
}

struct ras_storage_s *
ras_storage_new(struct ras_storage_options_s options) {
  struct ras_storage_s *storage = ras_storage_alloc();
  if (ras_storage_init(storage, options) < 0) {
    ras_free(storage);
    storage = 0;
  } else {
    storage->alloc = 1;
  }

  ras_emitter_init(&storage->emitter);
  return storage;
}

void
ras_storage_free(struct ras_storage_s *storage) {
  if (0 != storage && 1 == storage->alloc) {
    ras_free(storage);
  }
}

static int
ras_storage_destroy_after(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  struct ras_storage_s *storage = request->storage;

  if (0 != request) {
    request->storage = 0;
  }

  if (0 != storage) {
    for (int i = 0; i < RAS_STORAGE_MAX_REQUEST_QUEUE; ++i) {
      if (0 != storage->queue[i]) {
        storage->queue[i]->storage = 0;
        ras_request_free(storage->queue[i]);
      }
    }

    storage->queued = 0;
    ras_storage_free(storage);
  }

  return 0;
}

int
ras_storage_destroy(
  struct ras_storage_s *storage,
  ras_storage_destroy_callback_t *callback
) {
  return ras_storage_destroy_shared(storage, callback, 0, 0);
}

int
ras_storage_destroy_shared(
  struct ras_storage_s *storage,
  ras_storage_destroy_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);
  require(0 == ras_storage_close(storage, 0), errno);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .after = ras_storage_destroy_after,
      .type = RAS_REQUEST_DESTROY,
      .hook = hook,
      .data = 0
    });

  require(request, EFAULT);
  return queue_and_run(storage, request);
}
static int
ras_storage_open_emit(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  if (0 == request) {
    return 0;
  }

  struct ras_storage_s *storage = request->storage;

  if (0 == storage) {
    return 0;
  }

  if (1 == storage->destroyed) {
    return 0;
  }

  if (0 == err) {
    ras_emitter_emit(&request->storage->emitter, RAS_EVENT_OPEN, 0);
  } else {
    ras_emitter_emit(&request->storage->emitter, RAS_EVENT_ERROR, &err);
  }

  return 0;
}

int
ras_storage_open(
  struct ras_storage_s *storage,
  ras_storage_open_callback_t *callback
) {
  return ras_storage_open_shared(storage, callback, 0, 0);
}

int
ras_storage_open_shared(
  struct ras_storage_s *storage,
  ras_storage_open_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .emit = ras_storage_open_emit,
      .hook = hook,
      .type = RAS_REQUEST_OPEN,
      .data = 0,
    });

  require(request, EFAULT);
  return queue_and_run(storage, request);
}

static int
ras_storage_close_emit(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  if (0 == err) {
    ras_emitter_emit(&request->storage->emitter, RAS_EVENT_CLOSE, 0);
  } else {
    ras_emitter_emit(&request->storage->emitter, RAS_EVENT_ERROR, &err);
  }
  return 0;
}

int
ras_storage_close(
  struct ras_storage_s *storage,
  ras_storage_close_callback_t *callback
) {
  return ras_storage_close_shared(storage, callback, 0, 0);
}

int
ras_storage_close_shared(
  struct ras_storage_s *storage,
  ras_storage_close_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .after = ras_storage_close_emit,
      .hook = hook,
      .type = RAS_REQUEST_CLOSE,
      .data = 0,
    });

  require(request, EFAULT);
  return queue_and_run(storage, request);
}

static int
ras_storage_read_before(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  request->data = ras_alloc(size);
  memset(request->data, 0, size);
  return 0;
}

static int
ras_storage_read_after(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  if (0 != request) {
    if (0 != value && value != request->data) {
      ras_free(value);
    }

    ras_free(request->data);
    request->data = 0;

    if (1 != request->pending) {
      ras_request_free(request);
    }
  }
  return 0;
}

int
ras_storage_read(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_read_callback_t *callback
) {
  return ras_storage_read_shared(storage, offset, size, callback, 0, 0);
}

int
ras_storage_read_shared(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_read_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .offset = offset,
      .before = ras_storage_read_before,
      .after = ras_storage_read_after,
      .hook = hook,
      .type = RAS_REQUEST_READ,
      .size = size,
      .data = 0,
    });

  require(request, EFAULT);
  return run_request(storage, request);
}

static int
ras_storage_write_before(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  return 0;
}

static int
ras_storage_write_after(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  if (0 != request && 1 != request->pending) {
    ras_request_free(request);
  }
  return 0;
}

int
ras_storage_write(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  const void *buffer,
  ras_storage_write_callback_t *callback
) {
  return ras_storage_write_shared(storage, offset, size, buffer, callback, 0, 0);
}

int
ras_storage_write_shared(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  const void *buffer,
  ras_storage_write_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .offset = offset,
      .before = ras_storage_write_before,
      .after = ras_storage_write_after,
      .hook = hook,
      .type = RAS_REQUEST_WRITE,
      .size = size,
      .data = (void *) buffer,
    });

  require(request, EFAULT);
  return run_request(storage, request);
}

static int
ras_storage_stat_before(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  request->size = sizeof(struct ras_storage_stats_s);
  request->data = ras_alloc(sizeof(struct ras_storage_stats_s));
  memset(request->data, 0, sizeof(struct ras_storage_stats_s));
  return 0;
}

static int
ras_storage_stat_after(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  if (0 != request) {
    ras_free(request->data);
    request->data = 0;

    if (1 != request->pending) {
      ras_request_free(request);
    }
  }
  return 0;
}

int
ras_storage_stat(
  struct ras_storage_s *storage,
  ras_storage_stat_callback_t *callback
) {
  return ras_storage_stat_shared(storage, callback, 0, 0);
}

int
ras_storage_stat_shared(
  struct ras_storage_s *storage,
  ras_storage_stat_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .before = ras_storage_stat_before,
      .after = ras_storage_stat_after,
      .hook = hook,
      .type = RAS_REQUEST_STAT,
    });

  require(request, EFAULT);
  return run_request(storage, request);
}

static int
ras_storage_delete_before(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  return 0;
}

static int
ras_storage_delete_after(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  if (0 != request && 1 != request->pending) {
    ras_request_free(request);
  }
  return 0;
}

int
ras_storage_delete(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_delete_callback_t *callback
) {
  return ras_storage_delete_shared(storage, offset, size, callback, 0, 0);
}

int
ras_storage_delete_shared(
  struct ras_storage_s *storage,
  unsigned long int offset,
  unsigned long int size,
  ras_storage_delete_callback_t *callback,
  ras_request_callback_t *hook,
  void *shared
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .shared = shared,
      .offset = offset,
      .before = ras_storage_delete_before,
      .after = ras_storage_delete_after,
      .hook = hook,
      .type = RAS_REQUEST_DELETE,
      .size = size,
    });

  require(request, EFAULT);
  return run_request(storage, request);
}

struct ras_request_s *
ras_storage_queue_shift(struct ras_storage_s *storage) {
  struct ras_request_s *head = 0;

  if (0 == storage) {
    return 0;
  }

  if (storage->queued > 0) {
    head = storage->queue[0];
  }

  // shift
  for (int i = 0; i < storage->queued; ++i) {
    storage->queue[i] = storage->queue[i + 1];
  }

  for (int i = storage->queued; i < RAS_STORAGE_MAX_REQUEST_QUEUE; ++i) {
    storage->queue[i] = 0;
  }

  (void) --storage->queued;
  if (storage->queued < 0) {
    storage->queued = 0;
  }

  return head;
}

int
ras_storage_queue_push(
  struct ras_storage_s *storage,
  struct ras_request_s *request
) {
  require(storage, EFAULT);
  require(request, EFAULT);
  require(storage == request->storage, EINVAL);

  if ((int) storage->queued < 0) {
    storage->queued = 0;
  }

  // push
  storage->queue[storage->queued++] = request;
  request->pending = 1;
  return storage->queued;
}
