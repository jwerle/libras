#ifndef RAS_VERSION_H
#define RAS_VERSION_H

#include "platform.h"

RAS_EXPORT const char *
ras_version_string();

RAS_EXPORT const unsigned long int
ras_version();

RAS_EXPORT const unsigned long int
ras_version_major();

RAS_EXPORT const unsigned long int
ras_version_minor();

RAS_EXPORT const unsigned long int
ras_version_patch();

RAS_EXPORT const unsigned long int
ras_version_revision();

#endif
