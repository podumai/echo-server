#pragma once

#include <boost/asio.hpp>
#include <memory>

/**
 * @namespace tcp
 */
namespace tcp
{

/**
 * @class Session
 * @brief Session provides abstraction of client-server communication.
 */
class Session final : public std::enable_shared_from_this<Session>
{
 public:
  /**
   * @public
   * @brief Parameterized contructor for Session class.
   * 
   * @param[in] socket Socket for communication with peer.
   */
  Session(boost::asio::ip::tcp::socket&& socket);

 private:
  /**
   * @private
   * @brief Class method that initiates async read operation.
   * @details After successful read operaation this method
   *          initiates async write to the peer.
   */
  auto AsyncRead() -> void;

  /**
   * @private
   * @brief Class method that initiates async write operation.
   * @details After successful write operation this method
   *          initiates async read operation to the peer.
   */
  auto AsyncWrite() -> void;

 public:
  /**
   * @public
   * @brief Starts the half-duplex communication.
   */
  auto Start() -> void;

 private:
  boost::asio::ip::tcp::socket socket_;
  boost::asio::streambuf buffer_;
};

}  // namespace tcp