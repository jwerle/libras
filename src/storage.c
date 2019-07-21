#include "ras/allocator.h"
#include "ras/storage.h"
#include "require.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static void
run_request(
  struct ras_storage_s *storage,
  struct ras_request_s *request
) {
  if (1 == storage->needs_open) {
    ras_storage_open(storage, 0);
  }

  if (storage->queued > 0) {
    ras_storage_queue_push(storage, request);
  } else {
    ras_request_run(request);
  }
}

static void
queue_and_run(
  struct ras_storage_s *storage,
  struct ras_request_s *request
) {
  ras_storage_queue_push(storage, request);

  if (0 == storage->pending) {
    ras_request_run(request);
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
  require(memcpy(&storage->options, &options, sizeof(struct ras_storage_options_s)), EFAULT);
  storage->data = options.data;
  storage->needs_open = 1;
  storage->readable = 0 != options.read;
  storage->writable = 0 != options.write;
  storage->statable = 0 != options.stat;
  storage->deletable = 0 != options.del;
  storage->prefer_read_only = 0 != options.open_read_only;
  return 0;
}

struct ras_storage_s *
ras_storage_new(struct ras_storage_options_s options) {
  struct ras_storage_s *storage = ras_storage_alloc();
  if (ras_storage_init(storage, options) < 0) {
    ras_free(storage);
    return 0;
  } else {
    return storage;
  }
}

void
ras_storage_free(struct ras_storage_s *storage) {
  if (0 != storage) {
    ras_free(storage);
    storage = 0;
  }
}

static int
destroy_storage(
  struct ras_request_s *request,
  unsigned int err,
  void *value,
  unsigned long int size
) {
  ras_storage_free(request->storage);
  request->storage = 0;
  return 0;
}

int
ras_storage_destroy(
  struct ras_storage_s *storage,
  ras_storage_destroy_callback_t *callback
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .after = destroy_storage,
      .type = RAS_REQUEST_DESTROY,
      .data = 0
    });

  queue_and_run(storage, request);
  return 0;
}

int
ras_storage_open(
  struct ras_storage_s *storage,
  ras_storage_open_callback_t *callback
) {
  require(storage, EFAULT);

  if (1 == storage->opened && 0 == storage->needs_open) {
    if (0 != callback) {
      callback(storage, 0);
    }

    return 0;
  }

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .type = RAS_REQUEST_OPEN,
      .data = 0,
    });

  queue_and_run(storage, request);
  return 0;
}

int
ras_storage_close(
  struct ras_storage_s *storage,
  ras_storage_close_callback_t *callback
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .type = RAS_REQUEST_CLOSE,
      .data = 0,
    });

  queue_and_run(storage, request);
  return 0;
}

static int
ras_storage_read_before(
  struct ras_request_s *request,
  unsigned int err,
  void *value,
  unsigned long int size
) {
  value = request->data = ras_alloc(size);
  return 0;
}

static int
ras_storage_read_after(
  struct ras_request_s *request,
  unsigned int err,
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
  unsigned int offset,
  unsigned long int size,
  ras_storage_read_callback_t *callback
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .offset = offset,
      .before = ras_storage_read_before,
      .after = ras_storage_read_after,
      .type = RAS_REQUEST_READ,
      .size = size,
      .data = 0,
    });

  run_request(storage, request);
  return 0;
}

static int
ras_storage_write_before(
  struct ras_request_s *request,
  unsigned int err,
  void *value,
  unsigned long int size
) {
  return 0;
}

static int
ras_storage_write_after(
  struct ras_request_s *request,
  unsigned int err,
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
  unsigned int offset,
  unsigned long int size,
  const void *buffer,
  ras_storage_write_callback_t *callback
) {
  require(storage, EFAULT);

  struct ras_request_s *request = ras_request_new(
    (struct ras_request_options_s) {
      .callback = callback,
      .storage = storage,
      .offset = offset,
      .before = ras_storage_write_before,
      .after = ras_storage_write_after,
      .type = RAS_REQUEST_WRITE,
      .size = size,
      .data = (void *) buffer,
    });

  run_request(storage, request);
  return 0;
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
  for (int i = 1; i < storage->queued; ++i) {
    storage->queue[i - 1] = storage->queue[i];
  }

  (void) --storage->queued;
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
  // push
  storage->queue[storage->queued++] = request;
  request->pending = 1;
  return storage->queued;
}
