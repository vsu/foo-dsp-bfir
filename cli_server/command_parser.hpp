//
// client_msg_parser.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Victor C. Su
//

#ifndef CLI_COMMAND_PARSER_HPP
#define CLI_COMMAND_PARSER_HPP

#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>

#define CMD_DELIM  ' '
#define CMD_TERM   '\r'

namespace cli
{
namespace server
{

struct command;

/// Parser for incoming commands.
class command_parser
{
public:
    /// Construct ready to parse the command.
    command_parser();

    /// Reset to initial parser state.
    void reset();

    /// Parse some data. The tribool return value is true when a complete request
    /// has been parsed, false if the data is invalid, indeterminate when more
    /// data is required. The InputIterator return value indicates how much of the
    /// input has been consumed.
    template <typename InputIterator>
    boost::tuple<boost::tribool, InputIterator> parse(command& cmd,
            InputIterator begin, InputIterator end)
    {
        while (begin != end)
        {
            boost::tribool result = consume(cmd, *begin++);
            if (result || !result)
                return boost::make_tuple(result, begin);
        }

        boost::tribool result = boost::indeterminate;
        return boost::make_tuple(result, begin);
    }

private:
    /// Handle the next character of input.
    boost::tribool consume(command& cmd, char input);

    /// Check if a byte is a character.
    bool is_char(int c);

    /// The current state of the parser.
    enum state
    {
        op_start,
        op,
        data_start,
        data
    } state_;
};

} // namespace server
} // namespace cli

#endif // CLI_COMMAND_PARSER_HPP