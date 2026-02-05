/*
 * ClaudeOS - Basic Type Definitions
 * Shared across all kernel components
 */

#ifndef _CLAUDEOS_TYPES_H
#define _CLAUDEOS_TYPES_H

/* Fixed-width integer types */
typedef unsigned char      uint8_t;
typedef signed char        int8_t;
typedef unsigned short     uint16_t;
typedef signed short       int16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;

/* Size types */
typedef uint64_t size_t;
typedef int64_t  ssize_t;

/* Boolean - only define if not already defined */
#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#endif
#ifndef true
#define true  1
#endif
#ifndef false
#define false 0
#endif
#endif

/* NULL pointer */
#define NULL ((void*)0)

/* Useful macros */
#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#endif /* _CLAUDEOS_TYPES_H */
