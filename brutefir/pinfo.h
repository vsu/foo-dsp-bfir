#ifdef __cplusplus
extern "C" {
#endif 

/*
 * (c) 2011 Victor Su
 * (c) 2001 Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _PINFO_H_
#define _PINFO_H_

#include <stdarg.h>

void set_print_callback(void (*_print_callback)(const char *message));
void pinfo(const char *format, ...);

#endif 

#ifdef __cplusplus
}
#endif 