#pragma once

#include <boost/asio.hpp>
#include <optional>

/**
 * @namespace tcp
 */
namespace tcp
{

/**
 * @class Server
 * @brief Class that provides abstraction over network communication.
 * @details Server uses tcp sockets for communication. It initializes
 *          acceptor on <localhost>:<10000> and listens for new connections.
 *          It uses nonblocking async read/write implementation of Session
 *          class to abstract low level I/O operations. Server echoes all
 *          the messages back to the peer.
 */
class Server final
{
 public:
  /**
   * @public
   * @brief Parameterized constructor for Server class.
   * 
   * @param[in] context Context to use for I/O operations.
   * @param[in] port Port that server will use for binding.
   */
  Server(
    boost::asio::io_context& context,  //
    boost::asio::ip::port_type port
  );

  /**
   * @public
   * @brief Starts the async accept operation.
   */
  auto AsyncAccept() -> void;

 private:
  boost::asio::io_context& context_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::optional<boost::asio::ip::tcp::socket> socket_;
};

}  // namespace tcp