#ifndef _RAS_COMMON_H
#define _RAS_COMMON_H

#include <errno.h>

#define require(op, code) { \
  if (0 == (op)) {          \
    errno = code;           \
    return -errno;          \
  }                         \
}

#endif
