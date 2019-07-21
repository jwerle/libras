#include "ras/allocator.h"
#include "ras/request.h"
#include "ras/storage.h"
#include "common.h"
#include <string.h>

enum {
  ERROR = -1,
  NONE = 0,
  OPEN = 1,
  _MAX = RAS_MAX_ENUM,
};

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
  struct ras_request_options_s options
) {
  require(request, EFAULT);
  require(options.storage, EINVAL);
  require(options.callback, EINVAL);
  require(memset(request, 0, sizeof(struct ras_request_s)), EFAULT);

  request->callback = ras_request_callback;
  request->storage = options.storage;
  request->offset = options.offset;
  request->before = options.before;
  request->after = options.after;
  request->type = options.type;
  request->data = options.data;
  request->done = options.callback;
  request->size = options.size;
  return 0;
}

struct ras_request_s *
ras_request_new(struct ras_request_options_s options) {
  struct ras_request_s *request = ras_request_alloc();
  if (ras_request_init(request, options) < 0) {
    ras_free(request);
    return 0;
  } else {
    return request;
  }
}

void
ras_request_free(struct ras_request_s *request) {
  if (0 != request) {
    ras_free(request);
    request = 0;
  }
}

int
ras_request_run(struct ras_request_s *request) {
  require(request, EFAULT);
  require(request->storage, EFAULT);

  request->storage->pending++;

  if (0 != request->before) {
    request->before(request, request->err, request->data, request->size);
  }

  switch (request->type) {
    case RAS_REQUEST_READ:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.read) {
          request->storage->options.read(request);
        }
      }
      break;

    case RAS_REQUEST_WRITE:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.write) {
          request->storage->options.write(request);
        }
      }
      break;

    case RAS_REQUEST_DELETE:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.del) {
          request->storage->options.del(request);
        }
      }
      break;

    case RAS_REQUEST_STAT:
      if (OPEN == readystate(request)) {
        if (0 != request->storage->options.stat) {
          request->storage->options.stat(request);
        }
      }
      break;

    case RAS_REQUEST_OPEN:
      if (1 == request->storage->opened && 0 == request->storage->needs_open) {
        ras_request_callback(request, 0, 0, 0);
      } else {
        request->storage->needs_open = 0;
        if (1 == request->storage->prefer_read_only) {
          request->storage->options.open_read_only(request);
        } else if (0 != request->storage->options.open) {
          request->storage->options.open(request);
        }
      }
      break;

    case RAS_REQUEST_CLOSE:
      if (1 == request->storage->closed || 0 == request->storage->opened) {
        ras_request_callback(request, 0, 0, 0);
      } else if (0 != request->storage->options.close) {
        request->storage->options.close(request);
      }
      break;

    case RAS_REQUEST_DESTROY:
      if (1 == request->storage->destroyed) {
        ras_request_callback(request, 0, 0, 0);
      } else if (0 != request->storage->options.destroy) {
        request->storage->options.destroy(request);
      }
      break;

    case RAS_REQUEST_NONE:
      ras_request_callback(request, 0, 0, 0);
      break;
  }

  return 0;
}

int
ras_request_dequeue(
  struct ras_request_s *request,
  unsigned int err
) {
  require(request, EFAULT);
  require(request->storage, EFAULT);

  // maybe open error?
  if (err > 0) {
    if (RAS_REQUEST_OPEN == request->type) {
      for (int i = 0; i < request->storage->queued; ++i) {
        request->storage->queue[i]->err = err;
      }
    }
  } else {
    switch (request->type) {
      case RAS_REQUEST_OPEN:
        if (0 == request->storage->opened) {
          request->storage->opened = 1;
          // @TODO(jwerle): HOOK(open)
        }
        break;

      case RAS_REQUEST_CLOSE:
        if (0 == request->storage->closed) {
          request->storage->closed = 1;
          // @TODO(jwerle): HOOK(close)
        }
        break;

      case RAS_REQUEST_DESTROY:
        if (0 == request->storage->destroyed) {
          request->storage->destroyed = 1;
          // @TODO(jwerle): HOOK(destroy)
        }
        break;

      default:
        // NOOP
        (void)(0);
    }
  }

  struct ras_request_s *head = request->storage->queue[0];
  unsigned int queued = request->storage->queued;
  if (queued > 0 && head == request) {
    ras_request_free(ras_storage_queue_shift(request->storage));
  }

  // drain queue
  if (0 == --request->storage->pending) {
    while (request->storage->queued > 0) {
      if (ras_request_run(request->storage->queue[0]) < 0) {
        break;
      }

      if (request->type >= RAS_REQUEST_OPEN) { // [ open, close, destroy ]
        break;
      } else {
        ras_request_free(ras_storage_queue_shift(request->storage));
      }
    }
  }

  return 0;
}

int
ras_request_callback(
  struct ras_request_s *request,
  unsigned int err,
  void *value,
  unsigned long int size
) {
  require(request, EFAULT);
  require(request->storage, EFAULT);

  struct ras_storage_s *storage = request->storage;
  unsigned int type = request->type;
  void *done = request->done;
  int rc = ras_request_dequeue(request, err);

  if (0 != rc) {
    return rc;
  }

  if (0 != done) {
    switch (type) {
      case RAS_REQUEST_READ:
        ((ras_storage_read_callback_t *)done)(storage, err, value, size);
        break;

      case RAS_REQUEST_WRITE:
        ((ras_storage_write_callback_t *)done)(storage, err);
        break;

      case RAS_REQUEST_DELETE:
        ((ras_storage_delete_callback_t *)done)(storage, err);
        break;

      case RAS_REQUEST_STAT:
        ((ras_storage_stat_callback_t *)done)(
          storage, err, (struct ras_storage_stats_s *) value);
        break;

      case RAS_REQUEST_OPEN:
        ((ras_storage_open_callback_t *)done)(storage, err);
        break;

      case RAS_REQUEST_CLOSE:
        ((ras_storage_close_callback_t *)done)(storage, err);
        break;

      case RAS_REQUEST_DESTROY:
        ((ras_storage_destroy_callback_t *)done)(storage, err);
        break;

      case RAS_REQUEST_NONE:
        break;
    }
  }

  for (int i = 0; i < request->storage->queued; ++i) {
    if (request == request->storage->queue[i]) {
      ras_request_free(request);
      request->storage->queue[i] = 0;
      break;
    }
  }

  if (0 != request->after) {
    request->after(request, err, value, size);
  }

  return 0;
}

