#include <ras/ras.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define MAX_THREADS 16
#define SHARED_MEMORY_BYTES 64

typedef struct thread_s thread_t;
typedef struct thread_context_s thread_context_t;

typedef void *(pthread_start_routine_t)(void *);

struct thread_s {
  pthread_t id;
  pthread_attr_t attr;
  unsigned int index;
  ras_request_t *request;
};

struct thread_context_s {
  thread_t **threads;
  unsigned int count;
  unsigned char *memory; // allocated in 'open()'
};

static void
forward(ras_request_t *request, pthread_start_routine_t *start) {
  thread_context_t *context = (thread_context_t *) request->storage->data;
  thread_t **threads = context->threads;
  thread_t *thread = malloc(sizeof(*thread));

  *thread = (thread_t){ 0 };
  thread->index = context->count;
  thread->request = request;
  context->threads[context->count++] = thread;

  pthread_attr_init(&thread->attr);
  pthread_create(&thread->id, &thread->attr, start, thread);
}

static void *
open_storage_thread(void *arg) {
  thread_t *thread = (thread_t *) arg;
  ras_request_t *request = thread->request;
  thread_context_t *context = (thread_context_t *) request->storage->data;

  if (0 == context->memory) {
    context->memory = malloc(SHARED_MEMORY_BYTES);
  }

  request->callback(request, 0, 0, 0);

  pthread_exit(0);
  return 0;
}

static void *
close_storage_thread(void *arg) {
  thread_t *thread = (thread_t *) arg;
  ras_request_t *request = thread->request;
  thread_context_t *context = (thread_context_t *) request->storage->data;

  if (0 != context->memory) {
    // zero out just because
    for (int i = 0; i < SHARED_MEMORY_BYTES; ++i) {
      context->memory[i] = 0;
    }
  }

  request->callback(request, 0, 0, 0);

  pthread_exit(0);
  return 0;
}

static void *
destroy_storage_thread(void *arg) {
  thread_t *thread = (thread_t *) arg;
  ras_request_t *request = thread->request;
  thread_context_t *context = (thread_context_t *) request->storage->data;

  if (0 != context->memory) {
    free(context->memory);
    context->memory = 0;
  }

  request->callback(request, 0, 0, 0);
  pthread_exit(0);
  return 0;
}

static void *
read_storage_thread(void *arg) {
  thread_t *thread = (thread_t *) arg;
  ras_request_t *request = thread->request;
  thread_context_t *context = (thread_context_t *) request->storage->data;

  unsigned char *buffer = request->data;
  unsigned int size = request->size < SHARED_MEMORY_BYTES
    ? request->size
    : SHARED_MEMORY_BYTES;

  for (int i = 0; i < size; ++i) {
    buffer[i] = context->memory[i + request->offset];
  }

  request->callback(request, 0, buffer, size);
  pthread_exit(0);
  return 0;
}

static void *
write_storage_thread(void *arg) {
  thread_t *thread = (thread_t *) arg;
  ras_request_t *request = thread->request;
  thread_context_t *context = (thread_context_t *) request->storage->data;

  unsigned char *buffer = request->data;
  unsigned int size = request->size < SHARED_MEMORY_BYTES
    ? request->size
    : SHARED_MEMORY_BYTES;

  for (int i = 0; i < size; ++i) {
    context->memory[i + request->offset] = buffer[i];
  }

  request->callback(request, 0, buffer, size);
  pthread_exit(0);
  return 0;
}

static void
open_storage(ras_request_t *request) {
  forward(request, open_storage_thread);
}

static void
close_storage(ras_request_t *request) {
  forward(request, close_storage_thread);
}

static void
destroy_storage(ras_request_t *request) {
  forward(request, destroy_storage_thread);
}

static void
read_storage(ras_request_t *request) {
  forward(request, read_storage_thread);
}

static void
write_storage(ras_request_t *request) {
  forward(request, write_storage_thread);
}

static void
onopen(ras_storage_t *storage, int err) {
  printf("onopen(err=%d)\n", err);
}

static void
onclose(ras_storage_t *storage, int err) {
  printf("onclose(err=%d)\n", err);
}

static void
ondestroy(ras_storage_t *storage, int err) {
  printf("ondestroy(err=%d)\n", err);
}

static void
onread(ras_storage_t *storage, int err, void *buf, unsigned long length) {
  unsigned char *buffer = buf;
  printf("onread(err=%d (%s), length=%lu)\n", err, strerror(err), length);
  if (length > 0) {
    for (int i = 0; i < length; ++i) {
      printf("%0.2x ", buffer[i]);
    }
    printf("\n");
  }

  ras_storage_destroy(storage, ondestroy);
}

static void
onwrite(ras_storage_t *storage, int err) {
  printf("onwrite(err=%d)\n", err);
  ras_storage_read(storage, 0, 5, onread);
}

int
main(void) {
  thread_context_t context = { 0 };
  ras_storage_t storage = { 0 };

  context.threads = malloc(MAX_THREADS * sizeof(context.threads));
  memset(context.threads, 0, sizeof(*context.threads));

  ras_storage_init(&storage,
    (ras_storage_options_t) {
      .data = &context,
      .open = open_storage,
      .read = read_storage,
      .write = write_storage,
      .close = close_storage,
      .destroy = destroy_storage
    });

  const char *buffer = "hello";
  //ras_storage_open(&storage, onopen);
  ras_storage_write(&storage, 0, strlen(buffer), buffer, onwrite);

  for (int i = 0; i < context.count; ++i) {
    thread_t *thread = context.threads[i];
    pthread_join(thread->id, 0);
    context.threads[i] = 0;
    free(thread);
  }


  free(context.threads);
  return 0;
}
