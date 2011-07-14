#ifdef __cplusplus
extern "C" {
#endif 

/*
 * (c) 2002-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SYSARCH_H_
#define _SYSARCH_H_

#define ALIGNMENT 16

/*
 * Find out CPU architecture
 */
#if defined(__i386__) || defined(_M_IX86)
#define __ARCH_IA32__
#define __LITTLE_ENDIAN__
#elif defined(__sparc__) || defined(__sparc)
#define __ARCH_SPARC__
#define __BIG_ENDIAN__
#else
#define __ARCH_GENERIC__
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#undef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#undef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#endif

#if (defined(__LITTLE_ENDIAN__) && defined(__BIG_ENDIAN__)) ||                 \
    (!defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__))
#error unknown byte order
#endif

#endif

#ifdef __cplusplus
}
#endif 
