#include <sys/types.h>
#include <sys/mman.h>
#include <ras/ras.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

typedef struct mmap_context_s mmap_context_t;
struct mmap_context_s {
  int fd;
  int page_size;
  const char *filename;
};

static void *
mmap_page(int index, ras_request_t *request, mmap_context_t *context, int prot, int flags) {
  return mmap(0,
    request->size,
    prot,
    flags,
    context->fd,
    (context->page_size * index));
}

static void
mmap_open(ras_request_t *request) {
  mmap_context_t *context = request->storage->data;
  context->fd = open(context->filename, O_CREAT | O_SYNC | O_RDWR, S_IRUSR | S_IWUSR);
  if (context->fd < 0) {
    request->callback(request, errno, 0, 0);
  } else {
    request->callback(request, 0, 0, 0);
  }
}

static void
mmap_close(ras_request_t *request) {
}

static void
mmap_destroy(ras_request_t *request) {
  mmap_context_t *context = request->storage->data;
  close(context->fd);
  context->fd = 0;
  request->callback(request, 0, 0, 0);
}

static void
mmap_read(ras_request_t *request) {
  mmap_context_t *context = request->storage->data;
  unsigned int page_size = context->page_size;
  unsigned char *data = request->data;

  unsigned int i = request->offset / page_size;
  unsigned int rel = request->offset - i * page_size;
  unsigned int size = 0;
  unsigned int start = 0;

  while (start < request->size) {
    unsigned char *page = mmap_page(i++,
      request,
      context,
      PROT_READ,
      MAP_SHARED);

    if ((int *) -1 == (int *) page) {
      request->callback(request, errno, 0, 0);
      return;
    }

    unsigned int avail = page_size - rel;
    unsigned int want = request->size - start;
    unsigned int end = avail < want ? avail : want;

    if (msync(page, end, MS_SYNC) < 0) {
      munmap(page, request->size);
      request->callback(request, errno, 0, 0);
      return;
    }

    for (int j = 0; j < end; ++j) {
      data[start + j] = page[rel + j];
    }

    start += end;
    size += end;
    rel = 0;

    if (munmap(page, request->size) < 0) {
      request->callback(request, errno, 0, 0);
      return;
    }
  }

  request->callback(request, 0, data, size);
}

static void
mmap_write(ras_request_t *request) {
  mmap_context_t *context = request->storage->data;
  unsigned int page_size = context->page_size;
  unsigned char *data = request->data;

  unsigned int i = request->offset / page_size;
  unsigned int rel = request->offset - i * page_size;
  unsigned int start = 0;
  unsigned int length = request->offset + request->size;

  while (start < request->size) {
    unsigned char *page = mmap_page(i++,
      request,
      context,
      PROT_WRITE,
      MAP_SHARED);

    if ((int *) -1 == (int *) page) {
      request->callback(request, errno, 0, 0);
      return;
    }

    unsigned int avail = page_size - rel;
    unsigned int end = avail < (request->size - start)
      ? start + avail
      : request->size;

    memcpy(page + rel, request->data + start, end - start);
    start = end;
    rel = 0;
    i++;

    if (munmap(page, request->size) < 0) {
      request->callback(request, errno, 0, 0);
      return;
    }
  }

  request->callback(request, 0, 0, 0);
}

static void
mmap_del(ras_request_t *request) {
  mmap_context_t *context = request->storage->data;
  unsigned int page_size = context->page_size;
  unsigned char *data = request->data;

  unsigned int i = request->offset / page_size;
  unsigned int rel = request->offset - i * page_size;
  unsigned int start = 0;
  unsigned int length = request->offset + request->size;

  while (start < request->size) {
    unsigned char *page = mmap_page(i++,
      request,
      context,
      PROT_WRITE,
      MAP_SHARED);

    if ((int *) -1 == (int *) page) {
      request->callback(request, errno, 0, 0);
      return;
    }

    unsigned int avail = page_size - rel;
    unsigned int end = avail < (request->size - start)
      ? start + avail
      : request->size;

    memset(page + rel, 0, end - start);
    start = end;
    rel = 0;
    i++;

    if (munmap(page, request->size) < 0) {
      request->callback(request, errno, 0, 0);
      return;
    }
  }

  request->callback(request, 0, 0, 0);
}

static void
onopen(ras_storage_t *storage, int err) {
  printf("onopen(err=[%d: %s])\n", err, strerror(err));
}

static void
onread(ras_storage_t *storage, int err, void *buf, unsigned long int size) {
  if (err > 0) {
    printf("onread(err=%d type=%d): %s\n",
        err,
        storage->last_request.type,
        strerror(err));
  } else {
    unsigned char *data = buf;
    for (int i = 0; i < size; ++i) {
      printf("%0.2x ", (unsigned char) data[i]);
    }
    printf("\n");
  }
}

static void
onwrite(ras_storage_t *storage, int err) {
  printf("onwrite(err=[%d: %s])\n", err, strerror(err));
}

static void
ondelete(ras_storage_t *storage, int err) {
  printf("ondelete(err=[%d: %s])\n", err, strerror(err));
}

static void
ondestroy(ras_storage_t *storage, int err) {
  printf("ondestroy(err=[%d: %s])\n", err, strerror(err));
}

int
main(int argc, const char **argv) {
  const char *filename = argv[1];

  if (0 == filename) {
    fprintf(stderr, "error: Missing filename\n");
    return -1;
  }

  printf("file: %s\n", filename);

  mmap_context_t context = { 0 };
  ras_storage_t storage = { 0 };

  context.page_size = getpagesize();
  context.filename = filename;

  ras_storage_init(&storage,
    (ras_storage_options_t) {
      .data = &context,
      .open = mmap_open,
      .write = mmap_write,
      .read = mmap_read,
      .del = mmap_del,
      .destroy = mmap_destroy
    });

  const char *buffer = "hello";
  ras_storage_open(&storage, onopen);
  ras_storage_write(&storage, 0, strlen(buffer), buffer, onwrite);
  ras_storage_delete(&storage, 2, 2, ondelete);
  ras_storage_read(&storage, 0, strlen(buffer), onread);
  ras_storage_destroy(&storage, ondestroy);

  return 0;
}
