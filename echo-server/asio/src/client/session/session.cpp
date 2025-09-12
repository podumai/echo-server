#include <client/session/session.hpp>

#define func auto

namespace net = boost::asio;

namespace tcp
{

Session::Session(
  net::ip::tcp::socket&& socket
)
  : socket_{std::move(socket)}  //
  , buffer_{1024}
{ }

func Session::AsyncRead() -> void
{
  net::async_read_until(
    socket_,
    buffer_,
    '\n',
    [self = shared_from_this()](boost::system::error_code error_code, size_t processed_bytes) -> void
    {
      if (error_code)
      {
        return;
      }
      self->AsyncWrite();
    }
  );
}

func Session::AsyncWrite() -> void
{
  net::async_write(
    socket_,
    buffer_,
    [self = shared_from_this()](boost::system::error_code error_code, size_t processed_bytes) -> void
    {
      if (error_code)
      {
        return;
      }
      self->buffer_.consume(processed_bytes);
      self->AsyncRead();
    }
  );
}

func Session::Start() -> void
{
  AsyncRead();
}

}  // namespace tcp