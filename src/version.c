#include "ras/version.h"

#define SX(x) #x
#define S(x) SX(x)

#ifndef RAS_VERSION
#define RAS_VERSION 0
#endif

#ifndef RAS_NAME
#define RAS_NAME "libras"
#endif

#ifndef RAS_VERSION_MAJOR
#define RAS_VERSION_MAJOR 0
#endif

#ifndef RAS_VERSION_MINOR
#define RAS_VERSION_MINOR 0
#endif

#ifndef RAS_VERSION_PATCH
#define RAS_VERSION_PATCH 0
#endif

#ifndef RAS_VERSION_REVISION
#define RAS_VERSION_REVISION 0
#endif

#ifndef RAS_DATE_COMPILED
#define RAS_DATE_COMPILED ""
#endif

const char *
ras_version_string() {
  return RAS_NAME
    "@" S(RAS_VERSION_MAJOR)
    "." S(RAS_VERSION_MINOR)
    "." S(RAS_VERSION_PATCH)
    "." S(RAS_VERSION_REVISION) " (" RAS_DATE_COMPILED ")";
}

const unsigned long int
ras_version() {
  return (const unsigned long int) RAS_VERSION;
}

const unsigned long int
ras_version_major() {
  return RAS_VERSION >> 24 & 0xff;
}

const unsigned long int
ras_version_minor() {
  return RAS_VERSION >> 16 & 0xff;
}

const unsigned long int
ras_version_patch() {
  return RAS_VERSION >> 8 & 0xff;
}

const unsigned long int
ras_version_revision() {
  return RAS_VERSION & 0xff;
}
