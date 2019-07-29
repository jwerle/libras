#include "ras/allocator.h"
#include "ras/request.h"
#include "ras/storage.h"
#include "require.h"
#include <string.h>

enum {
  ERROR = -1,
  NONE = 0,
  OPEN = 1,
  _MAX = RAS_MAX_ENUM,
};

static int nrequests = 0xf + 0;

static int
ras_request_dequeue(
  struct ras_request_s *request,
  struct ras_storage_s *storage,
  enum ras_request_type type,
  unsigned int err);

static int
readystate(struct ras_request_s *request) {
  require(request, EFAULT);
  require(request->storage, EFAULT);
  int rc = NONE;

  if (1 == request->storage->opened && 0 == request->storage->closed) {
    return OPEN;
  }

  if (0 == request->storage->opened) {
    if (0 == request->err) {
      request->err = 1;
      rc = ERROR;
    }
  }

  return rc;
}

struct ras_request_s *
ras_request_alloc() {
  return ras_alloc(sizeof(struct ras_request_s));
}

int
ras_request_init(
  struct ras_request_s *request,
  const struct ras_request_options_s options
) {
  require(request, EFAULT);
  require(options.storage, EINVAL);
  require(memset(request, 0, sizeof(struct ras_request_s)), EFAULT);

  request->callback = ras_request_callback;
  request->destroyed = 0;
  request->pending = 0;
  request->storage = options.storage;
  request->shared = options.shared;
  request->offset = options.offset;
  request->before = options.before;
  request->after = options.after;
  request->alloc = 0;
  request->emit = options.emit;
  request->hook = options.hook;
  request->type = options.type;
  request->data = options.data;
  request->done = options.callback;
  request->size = options.size;
  request->err = 0;
  request->id = ++nrequests;
  return 0;
}

struct ras_request_s *
ras_request_new(const struct ras_request_options_s options) {
  struct ras_request_s *request = ras_request_alloc();

  if (0 != request) {
    if (ras_request_init(request, options) < 0) {
      ras_free(request);
      request = 0;
    } else if (0 != request) {
      request->alloc = 1;
    }
  }

  return request;
}

void
ras_request_free(struct ras_request_s *request) {
  if (0 != request && 1 == request->alloc) {
    request->storage = 0;
    request->alloc = 0;
    memset(request, 0, sizeof(*request));
    ras_free(request);
    request = 0;
  }
}

int
ras_request_run(struct ras_request_s *request) {
  require(request, EFAULT);
  require(request->storage, EFAULT);

  if (0 != request->err) {
    return ras_request_callback(request, request->err, 0, 0);
  }

  request->storage->pending++;

  if (0 != request->before) {
    request->before(request, request->err, request->data, request->size);
  }

  memcpy(
    &(request->storage->last_request),
    request,
    sizeof(struct ras_request_s));

  request->storage->last_request.done = 0;
  request->storage->last_request.after = 0;
  request->storage->last_request.before = 0;
  request->storage->last_request.callback = 0;

  switch (request->type) {
    case RAS_REQUEST_READ:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.read) {
          request->storage->options.read(request);
        } else {
          return ras_request_callback(request, ENOSYS, 0, 0);
        }
      }
      break;

    case RAS_REQUEST_WRITE:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.write) {
          request->storage->options.write(request);
        } else {
          return ras_request_callback(request, ENOSYS, 0, 0);
        }
      }
      break;

    case RAS_REQUEST_DELETE:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.del) {
          request->storage->options.del(request);
        } else {
          return ras_request_callback(request, ENOSYS, 0, 0);
        }
      }
      break;

    case RAS_REQUEST_STAT:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.stat) {
          request->storage->options.stat(request);
        } else {
          return ras_request_callback(request, ENOSYS, 0, 0);
        }
      }
      break;

    case RAS_REQUEST_OPEN:
      if (1 == request->storage->opened && 0 == request->storage->needs_open) {
        return ras_request_callback(request, 0, 0, 0);
      } else {
        if (1 == request->storage->prefer_read_only) {
          request->storage->options.open_read_only(request);
        } else if (0 != request->storage->options.open) {
          request->storage->options.open(request);
        } else {
          return ras_request_callback(request, 0, 0, 0);
        }
      }
      break;

    case RAS_REQUEST_CLOSE:
      if (1 == request->storage->closed || 0 == request->storage->opened) {
        return ras_request_callback(request, 0, 0, 0);
      } else if (0 != request->storage->options.close) {
        request->storage->options.close(request);
      } else {
        return ras_request_callback(request, 0, 0, 0);
      }
      break;

    case RAS_REQUEST_DESTROY:
      if (1 == request->storage->destroyed) {
        return ras_request_callback(request, 0, 0, 0);
      } else if (0 != request->storage->options.destroy) {
        request->storage->options.destroy(request);
      } else {
        return ras_request_callback(request, 0, 0, 0);
      }
      break;

    case RAS_REQUEST_NONE:
      return ras_request_callback(request, ENOSYS, 0, 0);
      break;
  }

  return 0;
}

int
ras_request_dequeue(
  struct ras_request_s *request,
  struct ras_storage_s *storage,
  enum ras_request_type type,
  unsigned int err
) {
  unsigned int needs_free = 0;
  require(request, EFAULT);
  require(request->storage, EFAULT);

  // maybe open error?
  if (err > 0) {
    if (RAS_REQUEST_OPEN == type) {
      storage->needs_open = 1;
      storage->opened = 0;
      for (int i = 0; i < storage->queued; ++i) {
        if (0 != storage->queue[i]) {
          storage->queue[i]->err = err;
        }
      }
    }
  } else {
    switch (type) {
      case RAS_REQUEST_OPEN:
        if (0 == storage->opened) {
          storage->opened = 1;
          storage->needs_open = 0;
          // @TODO(jwerle): HOOK(open)
        }
        break;

      case RAS_REQUEST_CLOSE:
        if (0 == storage->closed) {
          storage->opened = 0;
          storage->closed = 1;
          // @TODO(jwerle): HOOK(close)
        }
        break;

      case RAS_REQUEST_DESTROY:
        if (0 == storage->destroyed) {
          storage->destroyed = 1;
          request->destroyed = 1;
          // @TODO(jwerle): HOOK(destroy)
        }
        break;

      default:
        // NOOP
        (void)(0);
    }
  }

  struct ras_request_s *head = storage->queue[0];
  unsigned int queued = storage->queued;
  if (queued > 0 && head == request) {
    ras_storage_queue_shift(storage);
    needs_free = 1;
    request = 0;
    head = 0;
  }

  // drain queue
  if (0 == (int) --storage->pending) {
    while (storage->queued > 0) {
      if (0 == storage->queue[0]) {
        ras_storage_queue_shift(storage);
      }

      if (ras_request_run(storage->queue[0]) < 0) {
        break;
      }

      if (type >= RAS_REQUEST_OPEN) { // [ open, close, destroy ]
        break;
      } else {
        ras_request_free(ras_storage_queue_shift(storage));
      }
    }
  }

  if ((int) storage->pending < 0) {
    storage->pending = 0;
  }

  return needs_free;
}

int
ras_request_callback(
  struct ras_request_s *request,
  int err,
  void *value,
  unsigned long int size
) {
  require(request, EFAULT);
  require(request->storage, EFAULT);

  request->size = size;
  request->err = err;

  struct ras_storage_s *storage = request->storage;
  ras_request_callback_t *after = request->after;
  ras_request_callback_t *hook = request->hook;
  ras_request_callback_t *emit = request->emit;

  unsigned int opened = storage->opened;
  unsigned int closed = storage->closed;
  unsigned int type = request->type;
  void *done = request->done;

  if (0 != emit) {
    emit(request, err, value, size);
  }

  int needs_free = ras_request_dequeue(request, storage, type, err);
  unsigned int destroyed = storage->destroyed;

  if (type == RAS_REQUEST_OPEN && (1 == opened || 1 == destroyed)) {
    after = 0;
  }

  if (type == RAS_REQUEST_CLOSE && (1 == closed || 1 == destroyed)) {
    after = 0;
  }

#define CALL(T, ...)                                         \
  if (0 != hook) { hook(request, err, value, size); }        \
  if (0 != done) {  ((T) done)(storage, __VA_ARGS__); }      \
  if (0 != after) { after(request, err, value, size); }      \

  switch (type) {
    case RAS_REQUEST_READ:
      CALL(ras_storage_read_callback_t *, err, value, size);
      break;

    case RAS_REQUEST_WRITE:
      CALL(ras_storage_write_callback_t *, err);
      break;

    case RAS_REQUEST_DELETE:
      CALL(ras_storage_delete_callback_t *, err);
      break;

    case RAS_REQUEST_STAT:
      CALL(ras_storage_stat_callback_t *, err, (struct ras_storage_stats_s *) value);
      break;

    case RAS_REQUEST_OPEN:
      CALL(ras_storage_open_callback_t *, err);
      break;

    case RAS_REQUEST_CLOSE:
      CALL(ras_storage_close_callback_t *, err);
      break;

    case RAS_REQUEST_DESTROY:
      CALL(ras_storage_destroy_callback_t *, err);
      break;

    case RAS_REQUEST_NONE:
      break;
  }

#undef CALL

  if (1 == needs_free) {
    ras_request_free(request);
  }

  return - (int) err;
}
