/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _APP_PATH_HPP_
#define _APP_PATH_HPP_

#include <string>
#include "defs.h"

namespace app_path
{
    const std::wstring default_file_path = L"~\\brutefir";
    static std::wstring app_file_path = L"";

    std::wstring
    tilde_expansion(const std::wstring path);

    void
    set_path(const std::wstring path);

    std::wstring
    append_path(const std::wstring filename);

    std::wstring
    append_temp_path(const std::wstring filename);

    bool_t
    clean_path();
}

#endif
