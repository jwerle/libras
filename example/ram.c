#include <ras.h>

#ifndef RAS_RAM_MAX_BUFFERS
#define RAS_RAM_MAX_BUFFERS 16
#endif

typedef struct ram_s ram_t;
struct ram_s {
  RAS_STORAGE_FIELDS;
  unsigned long int length;
  unsigned int page_size;
  unsigned char *buffers[RAS_RAM_MAX_BUFFERS];
};

static void
del(ras_request_t *request) {
  // @TODO(jwerle)
}

static void
stat(ras_request_t *request) {
  ras_storage_stats_t stats = { 0 };
  ram_t *ram = (ram_t *) request->storage;
  stats.size = ram->length;
  request->callback(request, 0, &stats, 0);
}

static void
read(ras_request_t *request) {
  // @TODO(jwerle)
}

static void
write(ras_request_t *request) {
  // @TODO(jwerle)
}

static void
destroy(ras_request_t *request) {
  // @TODO(jwerle)
}

int
main (void) {
  ram_t ram = { 0 };
  ras_storage_init((ras_storage_t *) &ram,
    (ras_storage_options_t) {
      .del = del,
      .stat = stat,
      .read = read,
      .write = write,
      .destroy = destroy,
    });

  ram.page_size = 1024 * 1024;
  return 0;
}
