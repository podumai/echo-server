#include <server/server.hpp>
#include <client/session/session.hpp>

#define func auto

namespace net = boost::asio;

namespace tcp
{

Server::Server(
  net::io_context& context,  //
  net::ip::port_type port
)
  : context_{context}  //
  , acceptor_{context, net::ip::tcp::endpoint{net::ip::tcp::v4(), port}}
{ }

func Server::AsyncAccept() -> void
{
  socket_.emplace(context_);
  acceptor_.async_accept(
    *socket_,
    [this](boost::system::error_code error_code) -> void
    {
      if (error_code)
      {
        return;
      }
      std::make_shared<tcp::Session>(std::move(*socket_))->Start();
      AsyncAccept();
    }
  );
}

}  // namespace tcp