//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2011 Victor Su
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CLI_CONNECTION_HPP
#define CLI_CONNECTION_HPP

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/filesystem.hpp>
#include "command.hpp"
#include "command_parser.hpp"

namespace cli
{
namespace server
{

class connection_manager;

/// Represents a single connection from a client.
class connection
    : public boost::enable_shared_from_this<connection>,
  private boost::noncopyable
{
public:
    /// Construct a connection with the given io_service.
    explicit connection(boost::asio::io_service& io_service,
                        connection_manager& manager,
                        const std::string& default_dir);

    /// Get the socket associated with the connection.
    boost::asio::ip::tcp::socket& socket();

    /// Start the first asynchronous operation for the connection.
    void start();

    /// Stop all asynchronous operations associated with the connection.
    void stop();

private:
    /// Handles an incoming command.
    void handle_command(command& cmd);

    /// Handle completion of a read operation.
    void handle_read(const boost::system::error_code& e,
                     std::size_t bytes_transferred);

    /// Configures the native socket.
    void configure_socket();

    /// Sends data to the native socket.
    bool send_data(std::string data);

    /// Disconnects the client connection.
    void disconnect_client();

    /// Parses a string to an integer.
    static bool parse_int(std::string str, int &val);

    /// Adds the client to the HTTP server index file.
    /// Socket for the connection.
    boost::asio::ip::tcp::socket socket_;

    /// The manager for this connection.
    connection_manager& connection_manager_;

    /// Buffer for incoming data.
    boost::array<char, 8192> buffer_;

    /// The incoming command.
    command command_;

    /// The parser for the commmand.
    command_parser command_parser_;

    /// The underlying native socket.
    SOCKET native_socket_;

    /// The current directory.
    std::string current_dir_;
};

typedef boost::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace cli

#endif // CLI_CONNECTION_HPP