#ifdef __cplusplus
extern "C" {
#endif 

/*
 * (c) 2011 Victor Su
 * (c) 2000, 2002, 2004, 2006 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include <stdint.h>
#include <intrin.h>

static inline void
timestamp(volatile uint64_t *ts)
{
    *ts = __rdtsc();
}

#endif

#ifdef __cplusplus
}
#endif 
