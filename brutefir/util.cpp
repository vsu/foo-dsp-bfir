/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>

#include "util.hpp"

namespace util
{
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

    std::vector<std::string> &
    split(const std::string &s, 
          char delim, 
          std::vector<std::string> &elems) 
    {
        std::stringstream ss(s);
        std::string item;

        while(std::getline(ss, item, delim)) 
        {
            elems.push_back(item);
        }

        return elems;
    }

    std::vector<std::wstring> &
    split(const std::wstring &s, 
          wchar_t delim, 
          std::vector<std::wstring> &elems) 
    {
        std::wstringstream ss(s);
        std::wstring item;

        while(std::getline(ss, item, delim)) 
        {
            elems.push_back(item);
        }

        return elems;
    }

    std::vector<std::string>
    split(const std::string &s, 
          char delim) 
    {
        std::vector<std::string> elems;
        return split(s, delim, elems);
    }

    std::vector<std::wstring>
    split(const std::wstring &s, 
          wchar_t delim) 
    {
        std::vector<std::wstring> elems;
        return split(s, delim, elems);
    }
}