#include <ras/allocator.h>
#include <ras/storage.h>
#include <ras/version.h>
#include <assert.h>
#include <stdio.h>
#include <ok/ok.h>

#ifndef OK_EXPECTED
#define OK_EXPECTED 0
#endif

static const unsigned char buffer[4] = { 0xaa, 0xbb, 0xcc, 0xdd };
static unsigned char memory[32] = {
  0xde, 0xad, 0xfa, 0xce,
  0xbe, 0xef, 0xbe, 0xef,
  0x5e, 0x14, 0x9a, 0x7e,
  0xb4, 0xdd, 0x40, 0x57,
  0xad, 0x60, 0xbb, 0x15,
  0x62, 0x14, 0x8f, 0x0b,
  0x79, 0xb5, 0x19, 0xa2,
  0x3f, 0x0b, 0xd0, 0xb0
};

static void
open(struct ras_request_s *request) {
  ok("open()");
  request->callback(request, 0, 0, 0);
}

static void
close(struct ras_request_s *request) {
  ok("close()");
  request->callback(request, 0, 0, 0);
}

static void
destroy(struct ras_request_s *request) {
  ok("destroy()");
  request->callback(request, 0, 0, 0);
}

static void
read(struct ras_request_s *request) {
  ok("read()");
  unsigned char *data = request->data;
  int diff = 0;
  for (int i = 0; i < request->size; ++i) {
    data[i] = memory[i + request->offset];
    if (data[i] != buffer[i]) {
      (void) ++diff;
    }
  }

  if (0 == diff) {
    ok("read() from written buffer");
  }

  request->callback(request, 0, request->data, request->size);
}

static void
write(struct ras_request_s *request) {
  ok("write()");
  unsigned char *data = request->data;
  for (int i = 0; i < request->size; ++i) {
    memory[request->offset + i] = data[i];
  }
  request->callback(request, 0, 0, request->size);
}

static void
delete(struct ras_request_s *request) {
  ok("delete()");
  for (int i = 0; i < request->size; ++i) {
    memory[request->offset + i] = 0;
  }
  request->callback(request, 0, 0, request->size);
}

static void
stat(struct ras_request_s *request) {
  ok("stat()");
  struct ras_storage_stats_s *stats = request->data;
  stats->size = sizeof(memory);
  request->callback(request, 0, 0, request->size);
}

static void
onopen(struct ras_storage_s *storage, int err) {
  ok("onopen()");
}

static void
onclose(struct ras_storage_s *storage, int err) {
  ok("onclose()");
}

static void
ondestroy(struct ras_storage_s *storage, int err) {
  ok("ondestroy()");
}

static void
ondelete(struct ras_storage_s *storage, int err) {
  ok("ondelete()");
  for (int i = 0; i < storage->last_request.size; ++i) {
    assert(0 == memory[i + storage->last_request.offset]);
  }
}

static void
onstat(
  struct ras_storage_s *storage,
  int err,
  struct ras_storage_stats_s *stats
) {
  ok("ondelete()");
}

static void
onread(
  struct ras_storage_s *storage,
  int err,
  void *buffer,
  unsigned long int size
) {
  ok("onread()");
  unsigned char *data = buffer;
  for (int i = 0; i < size; ++i) {
    assert(memory[i + storage->last_request.offset] == data[i]);
    printf("%x ", data[i]);
  }
  printf("\n");
}

static void
onwrite(
  struct ras_storage_s *storage,
  int err
) {
  ok("onwrite()");
  unsigned char *data = storage->last_request.data;
  for (int i = 0; i < storage->last_request.size; ++i) {
    assert(data[i] == memory[i + storage->last_request.offset]);
  }
}

int
main(void) {
  printf("### ok: expecting %d\n", OK_EXPECTED);
  ok_expect(OK_EXPECTED);

  struct ras_storage_s *storage = ras_storage_new(
    (struct ras_storage_options_s) {
      .open = open,
      .close = close,
      .del = delete,
      .destroy = destroy,
      .stat = stat,
      .read = read,
      .write = write,
    });

  if (0 != storage) {
    ok("0 != storage");
  }

  if (0 == ras_storage_open(storage, onopen)) {
    ok("ras_storage_open()");
  }

  if (0 == ras_storage_write(storage, 15, 4, buffer, onwrite)) {
    ok("ras_storage_write()");
  }

  if (0 == ras_storage_read(storage, 15, 4, onread)) {
    ok("ras_storage_read()");
  }

  if (0 == ras_storage_delete(storage, 15, 4, ondelete)) {
    ok("ras_storage_delete()");
  }

  if (0 == ras_storage_stat(storage, onstat)) {
    ok("ras_storage_stat()");
  }

  if (0 == ras_storage_close(storage, onclose)) {
    ok("ras_storage_close()");
  }

  if (0 == ras_storage_destroy(storage, ondestroy)) {
    ok("ras_storage_destroy()");
  }

  const struct ras_allocator_stats_s stats = ras_allocator_stats();
  //printf("alloc=%d free=%d\n", stats.alloc, stats.free);
  if (stats.alloc == stats.free) {
    ok("stats.alloc == stats.free");
  }

  printf("%s\n", ras_version_string());
  ok_done();
  return ok_expected() - ok_count();
}
