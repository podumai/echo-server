#include <boost/asio.hpp>
#include <fmt/core.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <server/server.hpp>

namespace net = boost::asio;

namespace
{

constexpr int kConcurrencyHint{1};

}

auto main(
  int argc,  //
  char* argv[]
) -> int
{
  if (argc != 2 || std::string_view{argv[1]} == "--help")
  {
    fmt::print(stderr, "Usage: {} <port>\n", argv[0]);
    return 1;
  }
  net::ip::port_type server_port{static_cast<net::ip::port_type>(atoi(argv[1]))};
  net::io_context context{kConcurrencyHint};
  tcp::Server server{context, server_port};
  server.AsyncAccept();
  context.run();
  return 0;
}