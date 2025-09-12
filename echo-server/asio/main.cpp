#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <server/server.hpp>

namespace net = boost::asio;

namespace
{

constexpr unsigned kServerPort{10000U};
constexpr int kConcurrencyHint{1};

}

auto main(
  [[maybe_unused]] int argc,  //
  [[maybe_unused]] char* argv[]
) -> int
{
  net::io_context context{kConcurrencyHint};
  tcp::Server server{context, kServerPort};
  server.AsyncAccept();
  context.run();
  return 0;
}