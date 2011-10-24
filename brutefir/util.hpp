/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include <string>
#include <stdint.h>

// dB conversion macros
#define FROM_DB(db) (pow(10, (db) / 20.0))
#define TO_DB(val)  (20 * log10(val))

namespace util
{
    uint32_t 
    get_next_power_of_two(uint32_t value);
    
    uint32_t  
    get_next_multiple(uint32_t value,
                      uint32_t factor);

    std::wstring 
    str2wstr(std::string str);

    std::string 
    wstr2str(std::wstring wstr);
}

#endif
