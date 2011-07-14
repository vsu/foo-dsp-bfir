/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include "pinfo.h"

void (*print_callback)(const char *message) = NULL;

void set_print_callback(void (*_print_callback)(const char *message))
{
    print_callback = _print_callback;
}

void pinfo(const char *format, ...)
{
    char message[1024];

    va_list args;
    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);

    if (print_callback != NULL)
    {
        print_callback(message);
    }
}
