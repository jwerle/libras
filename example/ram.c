#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ras.h>

#ifndef RAS_RAM_MAX_BUFFERS
#define RAS_RAM_MAX_BUFFERS 1024
#endif

typedef struct ram_s ram_t;
struct ram_s {
  RAS_STORAGE_FIELDS;
  unsigned long int length;
  unsigned int page_size;
  unsigned char *buffers[RAS_RAM_MAX_BUFFERS];
};

static unsigned char *
ram_page(ram_t *ram, unsigned int i, unsigned int upsert) {
  unsigned char *page = ram->buffers[i];

  if (0 != page && 1 == upsert) {
    ras_free(page);
    ram->buffers[i] = 0;
    page = 0;
  }

  if (0 == page) {
    page = ras_alloc(ram->page_size);
  }

  ram->buffers[i] = page;
  return page;
}

static void
ram_del(ras_request_t *request) {
  // @TODO(jwerle)
}

static void
ram_stat(ras_request_t *request) {
  ras_storage_stats_t stats = { 0 };
  ram_t *ram = (ram_t *) request->storage;
  stats.size = ram->length;

  request->callback(request, 0, &stats, 0);
}

static void
ram_read(ras_request_t *request) {
  ram_t *ram = (ram_t *) request->storage;
  unsigned int i = request->offset / ram->page_size;
  unsigned int rel = request->offset - i * ram->page_size;
  unsigned int size = 0;
  unsigned int start = 0;

  if (request->offset + request->size > ram->length) {
    request->callback(request, ENOMEM, 0, 0);
    return;
  }

  while (start < request->size) {
    unsigned char *page = ram_page(ram, i++, 0);
    unsigned int avail = ram->page_size - rel;
    unsigned int want = request->size - start;
    unsigned int end = avail < want ? avail : want;

    if (0 != page) {
      memcpy(request->data + start, page + rel, end);
    }

    start += end;
    size += end;
    rel = 0;
  }

  request->callback(request, 0, request->data, size);
}

static void
ram_write(ras_request_t *request) {
  ram_t *ram = (ram_t *) request->storage;
  unsigned int i = request->offset / ram->page_size;
  unsigned int rel = request->offset - i * ram->page_size;
  unsigned int start = 0;
  unsigned int length = request->offset + request->size;

  if (length > ram->length) {
    ram->length = length;
  }

  while (start < request->size) {
    unsigned char *page = ram_page(ram, i++, 1);
    unsigned int avail = ram->page_size - rel;
    unsigned int end = avail < (request->size - start)
      ? start + avail
      : request->size;

    memcpy(page + rel, request->data + start, end - start);
    start = end;
    rel = 0;
  }

  request->callback(request, 0, 0, 0);
}

static void
ram_destroy(ras_request_t *request) {
  // @TODO(jwerle)
}

static void
onstat(ras_storage_t *storage, int err, ras_storage_stats_t *stats) {
  printf("onstat(): size=%lu \n", stats->size);
}

static void
onwrite(ras_storage_t *storage, int err) {
  if (err > 0) {
    printf("onwrite(err=%d type=%d): %s\n",
      err,
      storage->last_request.type,
      strerror(err));
  }
}

static void
onread(
  struct ras_storage_s *storage,
  int err,
  void *buffer,
  unsigned long int size
) {
  unsigned char *data = buffer;
  for (int i = 0; i < size; ++i) {
    printf("%x ", (unsigned char) data[i]);
  }
  printf("\n");
}

int
main (void) {
  ram_t ram = { 0 };
  ras_storage_init((ras_storage_t *) &ram,
    (ras_storage_options_t) {
      .del = ram_del,
      .stat = ram_stat,
      .read = ram_read,
      .write = ram_write,
      .destroy = ram_destroy,
    });

  ram.page_size = 1024 * 1024; // 1MB page size
  //ram.page_size = 4; // 4 byte page size

  const unsigned char buffer[8] = { 0xde, 0xad, 0xfa, 0xce, 0xfe, 0xfe, 0xfe, 0 };
  ras_storage_write((ras_storage_t *) &ram, 92, 8, buffer, onwrite);
  ras_storage_read((ras_storage_t *) &ram, 92, 8, onread);
  ras_storage_stat((ras_storage_t *) &ram, onstat);
  return 0;
}
