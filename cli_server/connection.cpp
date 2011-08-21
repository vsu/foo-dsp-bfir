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

#include "..\foo_dsp_bfir\common.h"
#include "connection.hpp"
#include "connection_manager.hpp"
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>
#include "..\brutefir\preprocessor.hpp"
#include "..\brutefir\util.hpp"


namespace cli
{
namespace server
{

connection::connection(boost::asio::io_service& io_service,
                       connection_manager& manager)
    : socket_(io_service),
      connection_manager_(manager)
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
            std::vector<std::string> mags = util::split(cfg_eq_mag.get_ptr(), ',');

            if (band < 0) band = 0;
            if (band > (int)mags.size() - 1) band = (int)mags.size() - 1;

            if (!cmd.data.empty())
            {
                if (parse_int(cmd.data, val))
                {
                    if (val < EQMagRangeMin) val = EQMagRangeMin;
                    if (val > EQMagRangeMax) val = EQMagRangeMax;

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
                    send_data("OK");
                }
                else
                {
                    send_data("ERR");
                }
            }
            else
            {
                send_data(cmd.op + " " + mags[band]);
            }
        }
        else
        {
            send_data("ERR");
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_eq_enable.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_file1_enable.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_file2_enable.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_file3_enable.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_eq_level.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_file1_level.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_file2_level.get_value()));
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
                send_data("OK");
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + 
                boost::lexical_cast<std::string>(cfg_file3_level.get_value()));
        }
    }
    else if (cmd.op == "F1FN")
    {
        if (!cmd.data.empty())
        {
            if (boost::filesystem::exists(cmd.data))
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
                    
                    send_data("OK");
                }
                else
                {
                    send_data("ERR");
                }
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + cfg_file1_filename.get_ptr());
        }
    }
    else if (cmd.op == "F2FN")
    {
        if (!cmd.data.empty())
        {
            if (boost::filesystem::exists(cmd.data))
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
                    
                    send_data("OK");
                }
                else
                {
                    send_data("ERR");
                }
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + cfg_file2_filename.get_ptr());
        }
    }
    else if (cmd.op == "F3FN")
    {
        if (!cmd.data.empty())
        {
            if (boost::filesystem::exists(cmd.data))
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
                    
                    send_data("OK");
                }
                else
                {
                    send_data("ERR");
                }
            }
            else
            {
                send_data("ERR");
            }
        }
        else
        {
            send_data(cmd.op + " " + cfg_file3_filename.get_ptr());
        }
    }
    else if (cmd.op == "F1MD")
    {
        send_data(cmd.op + " " + cfg_file1_metadata.get_ptr());
    }
    else if (cmd.op == "F2MD")
    {
        send_data(cmd.op + " " + cfg_file2_metadata.get_ptr());
    }
    else if (cmd.op == "F3MD")
    {
        send_data(cmd.op + " " + cfg_file3_metadata.get_ptr());
    }
    else if (cmd.op == "CLOSE")
    {
        disconnect_client();
    }
    else
    {
        send_data("ERR");
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

bool connection::send_data(std::string data)
{
    bool result = (send(native_socket_, data.c_str(), data.length(), 0) != SOCKET_ERROR);
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