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

    // Splits the specified string with the given delimiter
    // into the specified vector.
    //
    // Parameters:
    //   s      the string to split
    //   delim  the delimiter character to split on
    //   elems  a preconstructed vector
    //
    // Returns:
    //   A vector of split strings.
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

    // Splits the specified string with the given delimiter
    // into the specified vector.
    //
    // Parameters:
    //   s      the string to split
    //   delim  the delimiter character to split on
    //   elems  a preconstructed vector
    //
    // Returns:
    //   A vector of split strings.
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

    // Splits the specified string with the given delimiter
    // into a new vector.
    //
    // Parameters:
    //   s      the string to split
    //   delim  the delimiter character to split on
    //
    // Returns:
    //   A vector of split strings.
    std::vector<std::string>
    split(const std::string &s,
          char delim)
    {
        std::vector<std::string> elems;
        return split(s, delim, elems);
    }

    // Splits the specified string with the given delimiter
    // into a new vector.
    //
    // Parameters:
    //   s      the string to split
    //   delim  the delimiter character to split on
    //
    // Returns:
    //   A vector of split strings.
    std::vector<std::wstring>
    split(const std::wstring &s,
          wchar_t delim)
    {
        std::vector<std::wstring> elems;
        return split(s, delim, elems);
    }
}