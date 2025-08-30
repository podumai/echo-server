#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sync_server/errors/errors.h>
#include <sync_server/logger/logger.h>
#include <sync_server/server/server.h>
#include <sys/epoll.h>
#include <unistd.h>

static const int kInfiniteEpollTimeout = -1;
static const int kPthreadCreateSuccess = 0;
static const int kMessageBufferSize = 256;

int main(
  __attribute__((unused)) int argc,  //
  __attribute__((unused)) char* argv[]
)
{
  char message_buffer[kMessageBufferSize];
  int error_code;
  pthread_t leader_id = pthread_self();
  struct Server server;

  error_code = InitializeServerSockets(&server);
  if (error_code == kServerSocketInitFailed)
  {
    snprintf(
      message_buffer, //
      kMessageBufferSize,
      "Server initialization failed: errno[%d](%s)",
      errno,
      strerror(errno)
    );
    LOG_FATAL(message_buffer, leader_id);
  }

  int epfd = epoll_create1(EPOLL_CLOEXEC);
  if (epfd == kEpollCreateFailed)
  {
    snprintf(
      message_buffer, //
      kMessageBufferSize,
      "Server initialization failed: errno[%d](%s)",
      errno,
      strerror(errno)
    );
    LOG_FATAL(message_buffer, leader_id);
  }

  error_code = RegisterServerSockets(epfd, &server);
  if (error_code == kSocketRegistryFailed)
  {
    snprintf(
      message_buffer, //
      kMessageBufferSize,
      "Server initialization failed: epoll failed; errno[%d](%s)",
      errno,
      strerror(errno)
    );
    LOG_FATAL(message_buffer, leader_id);
  }

  PrintServerInitInfo(&server);

  int channels[2];
  error_code = pipe(channels);
  if (error_code == kPipeFailed)
  {
    snprintf(
      message_buffer, //
      kMessageBufferSize,
      "Server received error: pipe failed: [%d](%s)",
      errno,
      strerror(errno)
    );
    LOG_FATAL(message_buffer, leader_id);
  }

  struct sockaddr_in peer_info;
  memset(&peer_info, '\0', sizeof(struct sockaddr_in));
  socklen_t peer_info_size = (socklen_t) sizeof(struct sockaddr_in);

  struct epoll_event ep_events[SERVER_SOCKETS_COUNT];

  pthread_t control_block;
  error_code = pthread_create(&control_block, NULL, &ControlBlockFunction, (void*) channels[0]);
  if (error_code != kPthreadCreateSuccess)
  {
    snprintf(
      message_buffer, //
      kMessageBufferSize,
      "Server received error: pthread_create failed: [%d](%s)",
      errno,
      strerror(errno)
    );
    LOG_FATAL(message_buffer, leader_id);
  }

  while (true)
  {
    int ready_sockets = epoll_wait(epfd, ep_events, SERVER_SOCKETS_COUNT, kInfiniteEpollTimeout);
    if (ready_sockets == kEpollWaitFailed)
    {
      snprintf(
        message_buffer, //
        kMessageBufferSize,
        "Server received error: epoll failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, leader_id);
    }

    for (int i = 0; i < ready_sockets; ++i)
    {
      int clientfd = accept(ep_events[i].data.fd, (struct sockaddr*) &peer_info, &peer_info_size);
      if (clientfd == kAcceptFailed)
      {
        snprintf(
          message_buffer, //
          kMessageBufferSize,
          "Server received error: accept failed: [%d](%s)",
          errno,
          strerror(errno)
        );
        LOG_FATAL(message_buffer, leader_id);
      }

      snprintf(
        message_buffer, //
        kMessageBufferSize,
        "Server accepted connection on: %d\n\taddress: %s;\n\tport: %" PRIu16,
        clientfd,
        inet_ntoa(peer_info.sin_addr),
        ntohs(peer_info.sin_port)
      );
      LOG_INFO(message_buffer, leader_id);

      ssize_t processed_bytes = write(channels[1], &clientfd, sizeof(int));
      if (processed_bytes == kWriteFailed)
      {
        snprintf(
          message_buffer, //
          kMessageBufferSize,
          "Server received error: write failed: [%d](%s)",
          errno,
          strerror(errno)
        );
        LOG_FATAL(message_buffer, leader_id);
      }
    }
  }
  return 0;
}