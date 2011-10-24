//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Victor Su
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../foo_dsp_bfir/common.h"
#include "connection.hpp"
#include "connection_manager.hpp"
#include "command_parser.hpp"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
#include "../brutefir/preprocessor.hpp"
#include "../brutefir/util.hpp"
#include "../json_spirit/json_spirit.h"


namespace cli
{
namespace server
{

connection::connection(boost::asio::io_service& io_service,
                       connection_manager& manager,
                       const std::string& default_dir)
    : socket_(io_service),
      connection_manager_(manager),
      default_dir_(default_dir)
{
}

boost::asio::ip::tcp::socket& connection::socket()
{
    return socket_;
}

void connection::start()
{
    configure_socket();
    socket_.async_read_some(boost::asio::buffer(buffer_),
                            boost::bind(&connection::handle_read, shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
}

void connection::stop()
{
    socket_.close();
}

void connection::handle_command(command& cmd)
{
    int val;

    boost::to_upper(cmd.op);

    if (boost::starts_with(cmd.op, "EQM"))
    {
        int band;

        if (parse_int(cmd.op.substr(3), band))
        {
            std::vector<std::string> mags;    

            boost::algorithm::split(
                mags, 
                std::string(cfg_eq_mag.get_ptr()), 
                boost::is_any_of(","), 
                boost::algorithm::token_compress_on);
        
            if (band < 0) band = 0;
            if (band > (int)mags.size() - 1) band = (int)mags.size() - 1;

            if (!cmd.data.empty())
            {
                if (parse_int(cmd.data, val))
                {
                    if (val < EQLevelRangeMin) val = EQLevelRangeMin;
                    if (val > EQLevelRangeMax) val = EQLevelRangeMax;

                    mags[band] = boost::lexical_cast<std::string>(val);

                    // reassemble the magnitudes string
                    std::string str;
                    for (unsigned int ix = 0; ix < mags.size(); ix++)
                    {
                        str += mags[ix];

                        if (ix != (mags.size() - 1))
                        {
                            str += ",";
                        }
                    }

                    cfg_eq_mag.set_string(str.c_str());
                    send_reply(STATUS_OK);
                }
                else
                {
                    send_reply(STATUS_ERROR);
                }
            }
            else
            {
                send_reply(mags[band]);
            }
        }
        else
        {
            send_reply(STATUS_ERROR);
        }
    }
    else if (cmd.op == "EQEN")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < 0) val = 0;
                if (val > 1) val = 1;

                cfg_eq_enable = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_eq_enable.get_value()));
        }
    }
    else if (cmd.op == "F1EN")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < 0) val = 0;
                if (val > 1) val = 1;

                cfg_file1_enable = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_file1_enable.get_value()));
        }
    }
    else if (cmd.op == "F2EN")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < 0) val = 0;
                if (val > 1) val = 1;

                cfg_file2_enable = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_file2_enable.get_value()));
        }
    }
    else if (cmd.op == "F3EN")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < 0) val = 0;
                if (val > 1) val = 1;

                cfg_file3_enable = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_file3_enable.get_value()));
        }
    }
    else if (cmd.op == "EQLV")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < EQLevelRangeMin) val = EQLevelRangeMin;
                if (val > EQLevelRangeMax) val = EQLevelRangeMax;

                cfg_eq_level = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_eq_level.get_value()));
        }
    }
    else if (cmd.op == "F1LV")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < FileLevelRangeMin) val = FileLevelRangeMin;
                if (val > FileLevelRangeMax) val = FileLevelRangeMax;

                cfg_file1_level = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_file1_level.get_value()));
        }
    }
    else if (cmd.op == "F2LV")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < FileLevelRangeMin) val = FileLevelRangeMin;
                if (val > FileLevelRangeMax) val = FileLevelRangeMax;

                cfg_file2_level = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_file2_level.get_value()));
        }
    }
    else if (cmd.op == "F3LV")
    {
        if (!cmd.data.empty())
        {
            if (parse_int(cmd.data, val))
            {
                if (val < FileLevelRangeMin) val = FileLevelRangeMin;
                if (val > FileLevelRangeMax) val = FileLevelRangeMax;

                cfg_file3_level = val;
                send_reply(STATUS_OK);
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(boost::lexical_cast<std::string>(cfg_file3_level.get_value()));
        }
    }
    else if (cmd.op == "F1FN")
    {
        if (!cmd.data.empty())
        {
            if (cmd.data == FILENAME_NONE)
            {
                // Special filename indicating no file
                cfg_file1_filename.set_string("");
                cfg_file1_metadata.set_string("");
                cfg_file1_level = default_cfg_file1_level;
				cfg_file1_enable = 0;

                send_reply(STATUS_OK);
            }
            else if (boost::filesystem::exists(cmd.data))
            {
                int n_channels;
                int n_frames;
                int sampling_rate;
                double attenuation;

                // Calculate the optimum attentuation to prevent clipping
                if (preprocessor::calculate_attenuation(util::str2wstr(cmd.data),
                                                        FILTER_LEN,
                                                        REALSIZE,
                                                        &attenuation,
                                                        &n_channels,
                                                        &n_frames,
                                                        &sampling_rate))
                {
                    std::wstringstream info;

                    info << n_frames << L" samples, "
                         << n_channels << L" channels, "
                         << sampling_rate << L" Hz";

                    cfg_file1_filename.set_string(cmd.data.c_str());
                    cfg_file1_metadata.set_string((util::wstr2str(info.str())).c_str());
                    cfg_file1_level = (int)(attenuation * FILE_LEVEL_STEPS_PER_DB);
					cfg_file1_enable = 1;

                    send_reply(STATUS_OK);
                }
                else
                {
                    send_reply(STATUS_ERROR);
                }
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(cfg_file1_filename.get_ptr());
        }
    }
    else if (cmd.op == "F2FN")
    {
        if (!cmd.data.empty())
        {
            if (cmd.data == FILENAME_NONE)
            {
                // Special filename indicating no file
                cfg_file2_filename.set_string("");
                cfg_file2_metadata.set_string("");
                cfg_file2_level = default_cfg_file2_level;
				cfg_file2_enable = 0;

                send_reply(STATUS_OK);
            }
            else if (boost::filesystem::exists(cmd.data))
            {
                int n_channels;
                int n_frames;
                int sampling_rate;
                double attenuation;

                // Calculate the optimum attentuation to prevent clipping
                if (preprocessor::calculate_attenuation(util::str2wstr(cmd.data),
                                                        FILTER_LEN,
                                                        REALSIZE,
                                                        &attenuation,
                                                        &n_channels,
                                                        &n_frames,
                                                        &sampling_rate))
                {
                    std::wstringstream info;

                    info << n_frames << L" samples, "
                         << n_channels << L" channels, "
                         << sampling_rate << L" Hz";

                    cfg_file2_filename.set_string(cmd.data.c_str());
                    cfg_file2_metadata.set_string((util::wstr2str(info.str())).c_str());
                    cfg_file2_level = (int)(attenuation * FILE_LEVEL_STEPS_PER_DB);
					cfg_file2_enable = 1;

                    send_reply(STATUS_OK);
                }
                else
                {
                    send_reply(STATUS_ERROR);
                }
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(cfg_file2_filename.get_ptr());
        }
    }
    else if (cmd.op == "F3FN")
    {
        if (!cmd.data.empty())
        {
            if (cmd.data == FILENAME_NONE)
            {
                // Special filename indicating no file
                cfg_file3_filename.set_string("");
                cfg_file3_metadata.set_string("");
                cfg_file3_level = default_cfg_file3_level;
				cfg_file3_enable = 0;

                send_reply(STATUS_OK);
            }
            else if (boost::filesystem::exists(cmd.data))
            {
                int n_channels;
                int n_frames;
                int sampling_rate;
                double attenuation;

                // Calculate the optimum attentuation to prevent clipping
                if (preprocessor::calculate_attenuation(util::str2wstr(cmd.data),
                                                        FILTER_LEN,
                                                        REALSIZE,
                                                        &attenuation,
                                                        &n_channels,
                                                        &n_frames,
                                                        &sampling_rate))
                {
                    std::wstringstream info;

                    info << n_frames << L" samples, "
                         << n_channels << L" channels, "
                         << sampling_rate << L" Hz";

                    cfg_file3_filename.set_string(cmd.data.c_str());
                    cfg_file3_metadata.set_string((util::wstr2str(info.str())).c_str());
                    cfg_file3_level = (int)(attenuation * FILE_LEVEL_STEPS_PER_DB);
                    cfg_file3_enable = 1;

                    send_reply(STATUS_OK);
                }
                else
                {
                    send_reply(STATUS_ERROR);
                }
            }
            else
            {
                send_reply(STATUS_ERROR);
            }
        }
        else
        {
            send_reply(cfg_file3_filename.get_ptr());
        }
    }
    else if (cmd.op == "F1MD")
    {
        send_reply(cfg_file1_metadata.get_ptr());
    }
    else if (cmd.op == "F2MD")
    {
        send_reply(cfg_file2_metadata.get_ptr());
    }
    else if (cmd.op == "F3MD")
    {
        send_reply(cfg_file3_metadata.get_ptr());
    }
    else if (cmd.op == "DIR")
    {
        std:: stringstream out;

        boost::filesystem::path dir_path;

        // use default directory if no directory path specified
        dir_path = cmd.data.empty() 
            ? boost::filesystem::path(default_dir_)
            : boost::filesystem::path(cmd.data);

        try
        {
            if (dir_path.generic_string() == PATH_SUB_ROOT)
            {
                // list logical drives
                wchar_t buf[255];

                // get the drive letters as a set of strings
                int sz = GetLogicalDriveStrings(sizeof(buf), buf);

                if (sz > 0)
                {
                    // add subdirectories
                    json_spirit::Array subdir_array;

                    // buf now contains a list of all the drive letters. Each drive letter is
                    // terminated with '\0' and the last one is terminated by two consecutive '\0' bytes.
                    wchar_t * p1 = buf;
                    wchar_t * p2;
                    while (*p1 != '\0' && (p2 = wcschr(p1, '\0')) != NULL)
                    {
                        std::string drive_str = util::wstr2str(std::wstring(p1));

                        json_spirit::Object obj;
                        obj.push_back(json_spirit::Pair("display", drive_str));
                        obj.push_back(json_spirit::Pair("name", drive_str));
                        obj.push_back(json_spirit::Pair("path", drive_str));
                        subdir_array.push_back(obj);

                        p1 = p2 + 1;
                    }

                    // empty file array
                    json_spirit::Array file_array;

                    // create root object
                    json_spirit::Object root_obj;
                    root_obj.push_back(json_spirit::Pair("dir", ""));
                    root_obj.push_back(json_spirit::Pair("subdir", subdir_array));
                    root_obj.push_back(json_spirit::Pair("file", file_array));

                    // write to output
                    json_spirit::write_formatted(root_obj, out);
                }
                else
                {
                    send_reply(STATUS_ERROR);
                }
            }
            else
            {
                // use default directory if path does not exist
                if (!boost::filesystem::exists(dir_path))
                {
                    dir_path = boost::filesystem::path(default_dir_);
                }

                if (boost::filesystem::is_regular_file(dir_path))
                {
                    // regular file: just list the file
                    out << dir_path;
                }
                else if (boost::filesystem::is_directory(dir_path))
                {
                    // directory: iterate and list

                    // store paths to sort later
                    typedef std::vector<boost::filesystem::path> vec;
                    vec v;

                    std::copy(
                        boost::filesystem::directory_iterator(dir_path),
                        boost::filesystem::directory_iterator(),
                        std::back_inserter(v));

                    // sort since directory iteration may not be ordered
                    std::sort(v.begin(), v.end());

                    // add subdirectories
                    json_spirit::Array subdir_array;

                    // add parent directory (..) if it exists
                    boost::filesystem::path parent_path = dir_path.parent_path();

                    if (boost::filesystem::exists(parent_path))
                    {
                        json_spirit::Object obj;
                        obj.push_back(json_spirit::Pair("display", "[..]"));
                        obj.push_back(json_spirit::Pair("name", ".."));

                        // if the parent path ends with a colon, we are below
                        // the root directory of the drive, so indicate this with
                        // a special string.
                        if (boost::algorithm::ends_with(parent_path.generic_string(), ":"))
                        {
                            obj.push_back(json_spirit::Pair("path", PATH_SUB_ROOT));
                        }
                        else
                        {
                            obj.push_back(json_spirit::Pair("path", parent_path.generic_string()));
                        }

                        subdir_array.push_back(obj);
                    }

                    for (vec::const_iterator it(v.begin()), it_end(v.end()); it != it_end; ++it)
                    {
                    }

                    // add files and subdirectories
                    json_spirit::Array file_array;
                    for (vec::const_iterator it(v.begin()), it_end(v.end()); it != it_end; ++it)
                    {
                        DWORD dwAttrs;

                        dwAttrs = GetFileAttributes(it->wstring().c_str()); 
                        if (dwAttrs != INVALID_FILE_ATTRIBUTES)
                        {
                            if (!(dwAttrs & FILE_ATTRIBUTE_SYSTEM))
                            {
                                if (boost::filesystem::is_regular_file(*it))
                                {
                                    json_spirit::Object obj;
                                    obj.push_back(json_spirit::Pair("display", it->filename().generic_string()));
                                    obj.push_back(json_spirit::Pair("name", it->filename().generic_string()));
                                    obj.push_back(json_spirit::Pair("path", it->generic_string()));
                                    file_array.push_back(obj);
                                }
                                else if (boost::filesystem::is_directory(*it))
                                {
                                    json_spirit::Object obj;
                                    obj.push_back(json_spirit::Pair("display", it->filename().generic_string()));
                                    obj.push_back(json_spirit::Pair("name", it->filename().generic_string()));
                                    obj.push_back(json_spirit::Pair("path", it->generic_string()));
                                    subdir_array.push_back(obj);
                                }
                            }
                        }
                    }

                    // create root object
                    json_spirit::Object root_obj;
                    root_obj.push_back(json_spirit::Pair("dir", dir_path.generic_string()));
                    root_obj.push_back(json_spirit::Pair("subdir", subdir_array));
                    root_obj.push_back(json_spirit::Pair("file", file_array));

                    // write to output
                    json_spirit::write_formatted(root_obj, out);
                }
                else
                {
                    send_reply(STATUS_ERROR);
                }
            }
        }
        catch (const boost::filesystem::filesystem_error&)
        {
            send_reply(STATUS_ERROR);
        }

        send_reply(out.str());
    }
    else if (cmd.op == "CLOSE")
    {
        send_reply(STATUS_OK);
        disconnect_client();
    }
    else
    {
        send_reply(STATUS_ERROR);
    }
}

void connection::handle_read(const boost::system::error_code& e,
                             std::size_t bytes_transferred)
{
    if (!e)
    {
        boost::tribool result;
        boost::tie(result, boost::tuples::ignore) = command_parser_.parse(
                   command_, buffer_.data(), buffer_.data() + bytes_transferred);

        if (result)
        {
            handle_command(command_);
            command_.clear();
            command_parser_.reset();
        }
        else if (!result)
        {
            command_.clear();
            command_parser_.reset();
        }

        socket_.async_read_some(boost::asio::buffer(buffer_),
                                boost::bind(&connection::handle_read, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
    else if (e != boost::asio::error::operation_aborted)
    {
        connection_manager_.stop(shared_from_this());
    }
}

void connection::configure_socket()
{
    native_socket_ = socket_.native();
    if (native_socket_ != INVALID_SOCKET)
    {
        int optval = 1000;
        setsockopt(native_socket_, SOL_SOCKET, SO_SNDTIMEO, (const char *)&optval, sizeof(optval));
        setsockopt(native_socket_, SOL_SOCKET, SO_RCVTIMEO, (const char *)&optval, sizeof(optval));
    }
}

bool connection::send_reply( std::string data)
{
    std::string message = data + CMD_TERM;

    bool result = (send(native_socket_, message.c_str(), message.length(), 0) != SOCKET_ERROR);
    return result;
}

void connection::disconnect_client()
{
    connection_manager_.stop(shared_from_this());
}

bool connection::parse_int(std::string str, int &val)
{
    try
    {
        val = boost::lexical_cast<int>(str);
    }
    catch (const boost::bad_lexical_cast &)
    {
        return false;
    }

    return true;
}

} // namespace server
} // namespace cli