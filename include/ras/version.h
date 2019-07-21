#ifndef RAS_VERSION_H
#define RAS_VERSION_H

#include "platform.h"

/**
 * Returns the version string for the library.
 */
RAS_EXPORT const char *
ras_version_string();

/**
 * Returns the 32 bit version number that encodes the
 * major, minor, patch, and revision version parts.
 */
RAS_EXPORT const unsigned long int
ras_version();

/**
 * Returns the major version part.
 */
RAS_EXPORT const unsigned long int
ras_version_major();

/**
 * Returns the minor version part.
 */
RAS_EXPORT const unsigned long int
ras_version_minor();

/**
 * Returns the minor patch part.
 */
RAS_EXPORT const unsigned long int
ras_version_patch();

/**
 * Returns the minor revision part.
 */
RAS_EXPORT const unsigned long int
ras_version_revision();

#endif
