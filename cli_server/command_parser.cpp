//
// client_msg_parser.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Victor C. Su
//

#include "command_parser.hpp"
#include "command.hpp"

namespace cli
{
namespace server
{

command_parser::command_parser()
    : state_(op_start)
{
}

void command_parser::reset()
{
    state_ = op_start;
}

boost::tribool command_parser::consume(command& cmd, char input)
{
    switch (state_)
    {
    case op_start:
        if (!is_char(input))
        {
            return false;
        }
        else
        {
            state_ = op;
            cmd.op.push_back(input);
            return boost::indeterminate;
        }
    case op:
        if (input == ' ')
        {
            state_ = data_start;
            return boost::indeterminate;
        }
        else if (!is_char(input))
        {
            return false;
        }
        else if (input == '\r')
        {
            return true;
        }
        else
        {
            cmd.op.push_back(input);
            return boost::indeterminate;
        }
    case data_start:
        if (!is_char(input))
        {
            return false;
        }
        else
        {
            state_ = data;
            cmd.data.push_back(input);
            return boost::indeterminate;
        }
    case data:
        if (!is_char(input))
        {
            return false;
        }
        else if (input == '\r')
        {
            return true;
        }
        else
        {
            cmd.data.push_back(input);
            return boost::indeterminate;
        }
    }

    return false;
}

bool command_parser::is_char(int c)
{
    return c >= 0 && c <= 127;
}


} // namespace server
} // namespace cli