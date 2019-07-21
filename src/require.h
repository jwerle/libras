#ifndef _RAS_REQURE_H
#define _RAS_REQURE_H

#include <errno.h>

#define require(op, code) { \
  if (0 == (op)) {          \
    errno = code;           \
    return -errno;          \
  }                         \
}

#endif
