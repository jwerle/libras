#ifndef RAS_PLATFORM_H
#define RAS_PLATFORM_H

#if defined(_WIN32)
#  define RAS_EXPORT __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR) >= 303
#  define RAS_EXPORT __attribute__((visibility("default")))
#  define RAS_INLINE inline
#else
#  define RAS_EXPORT
#  define RAS_INLINE
#endif

#ifndef RAS_ALIGNMENT
#  define RAS_ALIGNMENT sizeof(unsigned long) // platform word
#endif

#ifndef RAS_MAX_ENUM
#  define RAS_MAX_ENUM 0x7FFFFFFF
#endif

#endif
