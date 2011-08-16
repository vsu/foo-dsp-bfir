//
// client_msg.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Victor C. Su
//

#ifndef CLI_COMMAND_HPP
#define CLI_COMMAND_HPP

#include <string>
#include <vector>

namespace cli
{
namespace server
{

/// A command received from a client.
struct command
{
    /// The command operation.
    std::string op;

    /// The command data.
    std::string data;

    /// Clears all fields of the command.
    void clear();
};

} // namespace server
} // namespace cli

#endif // CLI_COMMAND_HPP