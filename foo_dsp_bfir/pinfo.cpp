/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <foobar2000.h>
#include <stdarg.h>

extern "C" void pinfo(const char *format, ...);

void pinfo(const char *format, ...)
{
    char message[1024];

    va_list args;
    va_start(args, format);
    vsprintf(message, format, args);
    va_end(args);

    console::print(message);
}
