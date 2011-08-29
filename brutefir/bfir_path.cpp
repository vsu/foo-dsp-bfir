/*
 * (c) 2011 Victor Su
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#include <stdlib.h>
#include <string>

#include <boost/filesystem.hpp>

#include "bfir_path.hpp"
#include "defs.h"

namespace bfir_path
{
    // Expands the tilde (~) character into the user's home directory.
    //
    // Parameters:
    //   path   the file path
    //
    // Returns:
    //   The expanded path.
    std::wstring
    tilde_expansion(const std::wstring path)
    {
        std::wstring full_path;
        wchar_t *home_dir = NULL;
        size_t tilde_pos;
        size_t slash_pos;

        // return path if no tilde
        if ((tilde_pos = path.find(L"~")) == std::string::npos)
        {
            return path;
        }

        home_dir = _wgetenv(L"USERPROFILE");

        // return path if no home directory
        if (home_dir == NULL)
        {
            return path;
        }

        slash_pos = path.find_first_of(PATH_SEPARATOR_CHAR);

        // return home directory in the case where tilde is the character
        if (slash_pos == std::string::npos)
        {
            full_path.assign(home_dir);
            full_path.append(PATH_SEPARATOR_STR);
            return full_path;
        }

        // append the path to the home directory
        full_path.assign(home_dir);
        full_path.append(path.substr(slash_pos));
        return full_path;
    }

    // Sets the BruteFIR file path.
    //
    // Parameters:
    //   path  the file path
    void
    set_path(const std::wstring path)
    {
        // Copy the application file path
        if (!path.empty())
        {
            app_file_path.assign(path);
        }
    }

    // Appends the BruteFIR file path to the filename.
    //
    // Parameters:
    //   filename  the name of the file
    //
    // Returns:
    //   The filename appended to the application file path.
    std::wstring
    append_path(const std::wstring filename)
    {
        std::wstring full_path;

        if (app_file_path.empty())
        {
            full_path = tilde_expansion(default_file_path);
        }
        else
        {
            full_path = tilde_expansion(app_file_path);
        }

        try
        {
            boost::filesystem::create_directories(full_path);
        }
        catch(boost::filesystem::filesystem_error)
        {
            return filename;
        }

        full_path.append(PATH_SEPARATOR_STR);
        full_path.append(filename);

        return full_path;
    }

    // Appends the BruteFIR temporary file path to the filename.
    //
    // Parameters:
    //   filename  the name of the file
    //
    // Returns:
    //   The filename appended to the application temporary file path.
    std::wstring
    append_temp_path(std::wstring filename)
    {
        std::wstring full_path;

        if (app_file_path.empty())
        {
            full_path = tilde_expansion(default_file_path);
        }
        else
        {
            full_path = tilde_expansion(app_file_path);
        }

        full_path.append(PATH_SEPARATOR_STR);
        full_path.append(L"temp");

        try
        {
            boost::filesystem::create_directories(full_path);
        }
        catch(boost::filesystem::filesystem_error)
        {
            return filename;
        }

        full_path.append(PATH_SEPARATOR_STR);
        full_path.append(filename);

        return full_path;
    }

    // Cleans up the BruteFIR temporary file path.
    // Returns:
    //   True if successful, false otherwise.
    bool
    clean_path()
    {
        bool status = true;
        std::wstring full_path;

        if (app_file_path.empty())
        {
            full_path = tilde_expansion(default_file_path);
        }
        else
        {
            full_path = tilde_expansion(app_file_path);
        }

        full_path.append(PATH_SEPARATOR_STR);
        full_path.append(L"temp");

        try
        {
            boost::filesystem::remove_all(full_path);
        }
        catch(boost::filesystem::filesystem_error)
        {
            status = false;
        }

        return status;
    }
}