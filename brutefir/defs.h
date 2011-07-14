#ifdef __cplusplus
extern "C" {
#endif 

/*
 * (c) 2011 Victor Su
 * (c) 1999, 2002-2004 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _DEFS_H_
#define _DEFS_H_

#include "sysarch.h"

#define true ((bool_t)1)
#define false ((bool_t)0)
typedef int bool_t;

typedef unsigned long long ull_t;
typedef long long ll_t;

#define inline __inline

#define PATH_SEPARATOR_CHAR L'\\'
#define PATH_SEPARATOR_STR L"\\"

#endif

#ifdef __cplusplus
}
#endif 