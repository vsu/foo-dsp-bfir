/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <string>
#include <sstream>
#include <stdint.h>

#include "util.hpp"

namespace util
{
    // Returns the next power of two greater than the specified value.
    //
    // Parameters:
    //   value  a given value
    //
    // Returns:
    //   The next power of two greater than the specified value.
    uint32_t
    get_next_power_of_two(uint32_t value)
    {
        uint32_t next_power_of_two = 2;

        while (value > next_power_of_two)
        {
            next_power_of_two <<= 1;
        }

        return next_power_of_two;
    }

    // Returns the next multiple of the factor greater than
    // the specified value.
    //
    // Parameters:
    //   value   a given value
    //   factor  a given factor
    //
    // Returns:
    //   The next multiple of the factor greater than
    //   the specified value.
    uint32_t
    get_next_multiple(uint32_t value, uint32_t factor)
    {
        uint32_t multiple = factor;

        while (value > multiple)
        {
            multiple += factor;
        }

        return multiple;
    }

    // Converts a character string to a wide character string.
    //
    // Parameters:
    //   str  the character string to convert
    //
    // Returns:
    //   A wide character string.
    std::wstring 
    str2wstr(std::string str)
    {
        int size = mbstowcs(NULL, str.c_str(), 0);

        wchar_t * str_wcs = (wchar_t *) _alloca((size + 1) * sizeof(wchar_t));
        mbstowcs(str_wcs, str.c_str(), size + 1);

        return std::wstring(str_wcs);
    }

    // Converts a wide character string to a character string.
    //
    // Parameters:
    //   wstr  the wide character string to convert
    //
    // Returns:
    //   A character string.
    std::string 
    wstr2str(std::wstring wstr)
    {
        int size = wcstombs(NULL, wstr.c_str(), 0);

        char *str_mbs = (char *) _alloca((size + 1) * sizeof(char));
        wcstombs(str_mbs, wstr.c_str(), size + 1);

        return std::string(str_mbs);
    }

}