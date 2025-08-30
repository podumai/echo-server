#include <fmt/core.h>

#include <array>
#include <atomic>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

volatile std::atomic<unsigned> worker_count;
constexpr int kBufferSize{1024};

namespace ipv4
{

constexpr std::string_view kAddrLoopBack{"127.0.0.1"};

}

auto HandleConnection(
  std::shared_ptr<boost::asio::ip::tcp::socket> socket
) -> void
{
  ++worker_count;
  try
  {
    std::array<unsigned char, kBufferSize> buffer;
    while (true)
    {
      size_t processedBytes{socket->read_some(boost::asio::buffer(buffer))};
      boost::asio::write(*socket, boost::asio::buffer(buffer, processedBytes));
    }
  }
  catch (const std::exception& error)
  {
    fmt::print(stderr, "[ERROR] Message: {}\n", error.what());
  }
  --worker_count;
}

auto main(
  int argc,  //
  char* argv[]
) -> int
{
  if (argc != 2 || strcmp(argv[0], "--help") == 0)
  {
    fmt::print(stderr, "Usage: {} <port>\n", argv[0]);
    return 1;
  }

  unsigned max_worker_count{std::thread::hardware_concurrency()};
  if (max_worker_count == 0U)
  {
    max_worker_count = 2U;
  }

  boost::beast::net::io_context context;
  boost::beast::net::ip::tcp::endpoint endpoint{
    boost::beast::net::ip::make_address_v4(ipv4::kAddrLoopBack),
    static_cast<boost::beast::net::ip::port_type>(atoi(argv[1]))
  };
  boost::beast::net::ip::tcp::acceptor acceptor{context, endpoint};

  while (true)
  {
    auto connection{std::make_shared<boost::asio::ip::tcp::socket>(acceptor.accept())};
    while (worker_count.load() >= max_worker_count)
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(500ms);
    }
    std::thread{&HandleConnection, connection}.detach();
  }

  return 0;
}