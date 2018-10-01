// mimic openSSH-portable's includes.h file to be able to use
// its unmodified blowfish code

#define HAVE_BLF_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* activate extra prototypes for glibc */
#endif
#include <sys/types.h>

#ifdef _WIN32
#include <stdint.h>

typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
typedef uint8_t u_int8_t;

#define bzero(p, s) memset(p, 0, s)
#endif
