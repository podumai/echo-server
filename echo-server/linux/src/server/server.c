#include <sync_server/server/server.h>
#include <sync_server/errors/errors.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <sync_server/logger/logger.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MALLOC_FAILED NULL

const int kServerSocketInitFailed = -1;
const int kSocketRegistryFailed = -1;

static const int kSocketPendingConnections = 5;
static const int kSocketBufferAllocFailed = -1;
static const int kServerBasePort = 10000;
static const int kSocketBufferSize = 1024;
static const unsigned kWorkersCount = 4U;
static const int kMessageBufferSize = 256;
static const int kPthreadCreateSuccess = 0;

__attribute__((nonnull(1, 2))) __attribute__((warn_unused_result))
static int CreateSocket(
  struct Socket* sock, //
  struct sockaddr_in* sock_info,
  int buffer_size
)
{
  sock->socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sock->socketfd_ == kSocketFailed)
  {
    return kSocketFailed;
  }
  int error_code = bind(sock->socketfd_, (struct sockaddr*) sock_info, sizeof(struct sockaddr_in));
  if (error_code == kBindFailed)
  {
    return kBindFailed;
  }
  error_code = listen(sock->socketfd_, kSocketPendingConnections);
  if (error_code == kListenFailed)
  {
    return kListenFailed;
  }
  sock->buffer_.data_ = malloc(buffer_size);
  if (sock->buffer_.data_ == MALLOC_FAILED)
  {
    return kSocketBufferAllocFailed;
  }
  sock->buffer_.size_ = buffer_size;
  return 0;
}

int InitializeServerSockets(struct Server* server)
{
  LOG_DEBUG("InitializeServerSockets[1]: start sockets initialization", pthread_self());

  struct sockaddr_in server_addr;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_family = AF_INET;

  for (int i = 0; i < SERVER_SOCKETS_COUNT; ++i)
  {
    server_addr.sin_port = htons(kServerBasePort + i);
    int error_code = CreateSocket(server->sockets_ + i, &server_addr, kSocketBufferSize);
    if (error_code == kSocketFailed)
    {
      return kSocketFailed;
    }
  }

  LOG_DEBUG("InitializeServerSockets[2]: end sockets initialization", pthread_self());
  return 0;
}

int RegisterServerSockets(int epfd, struct Server* server)
{
  LOG_DEBUG("RegisterServerSockets[1]: start sockets registration", pthread_self());

  struct epoll_event ev;
  ev.events = EPOLLIN;
  for (int i = 0; i < SERVER_SOCKETS_COUNT; ++i)
  {
    ev.data.fd = server->sockets_[i].socketfd_;
    int error_code = epoll_ctl(epfd, EPOLL_CTL_ADD, server->sockets_[i].socketfd_, &ev);
    if (error_code == kEpollCtlFailed)
    {
      return kEpollCtlFailed;
    }
  }

  LOG_DEBUG("RegisterServerSockets[2]: end sockets registration", pthread_self());
  return 0;
}

void PrintServerInitInfo(struct Server* server)
{
  puts("Server initialized with:");
  printf(
    "\taddress: %s;\n"
    "\tports:\n",
    inet_ntoa(server->info_.sin_addr)
  );
  for (int i = 0; i < SERVER_SOCKETS_COUNT; ++i)
  {
    printf("\t\t- %hd;\n", kServerBasePort + i);
  }
  puts("\tsocket type: SOCK_STREAM\n"
       "\tprotocol: TCP/IP");
}

int ConfigureClientSocket(int clientfd)
{
  LOG_DEBUG("ConfigureClientSocket[1]: start configurating flags", pthread_self());

  int fd_flags = fcntl(clientfd, F_GETFL);
  if (fd_flags == kFcntlFailed)
  {
    return kFcntlFailed;
  }
  fd_flags |= O_NONBLOCK;
  int error_code = fcntl(clientfd, F_SETFL, fd_flags);
  if (error_code == kFcntlFailed)
  {
    return kFcntlFailed;
  }

  LOG_DEBUG("ConfigureClientSocket[2]: end configurating flags", pthread_self());
  return 0;
}

struct WorkerInfo
{
  pthread_t control_block_id_;
  int input_channel_;
};

static __thread char message_buffer[kMessageBufferSize];

static const int kWorkerBufferSize = 16;

__attribute__((nonnull(1))) static void* WorkerFunction(
  void* arg
)
{
  int error_code;
  unsigned char buffer[kWorkerBufferSize];
  struct WorkerInfo* worker_info = (struct WorkerInfo*) arg;
  pthread_t worker_id = pthread_self();
  int clientfd;

  while (true)
  {
    ssize_t processed_bytes = read(worker_info->input_channel_, &clientfd, sizeof(int));
    if (processed_bytes == kReadFailed)
    {
      snprintf(
        message_buffer, kMessageBufferSize, "Worker received error: read[1] failed: [%d](%s)", errno, strerror(errno)
      );
      LOG_FATAL(message_buffer, worker_id);
    }

    processed_bytes = 0;
    while (processed_bytes != kWorkerBufferSize)
    {
      ssize_t bytes = read(clientfd, buffer, kWorkerBufferSize - processed_bytes);
      if (bytes == kReadFailed)
      {
        snprintf(
          message_buffer, kMessageBufferSize, "Worker received error: read[2] failed: [%d](%s)", errno, strerror(errno)
        );
        LOG_FATAL(message_buffer, worker_id);
      }

      processed_bytes += bytes;
      bytes = write(clientfd, buffer, bytes);
      if (bytes == kWriteFailed)
      {
        snprintf(
          message_buffer, kMessageBufferSize, "Worker received error: write failed: [%d](%s)", errno, strerror(errno)
        );
        LOG_FATAL(message_buffer, worker_id);
      }
    }

    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);
    snprintf(
      message_buffer, kMessageBufferSize, "Worker: client qouta exceded. [%d] bytes processed.", kWorkerBufferSize
    );
    LOG_INFO(message_buffer, worker_id);
  }

  return NULL;
}

void* ControlBlockFunction(
  void* arg
)
{
  pthread_t control_block_id = pthread_self();
  int channels[kWorkersCount][2];
  for (int i = 0; i < kWorkersCount; ++i)
  {
    int error_code = pipe(channels[i]);
    if (error_code == kPipeFailed)
    {
      snprintf(
        message_buffer,
        kMessageBufferSize,
        "Control block received error: pipe failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, control_block_id);
    }
  }

  pthread_t workers[kWorkersCount];
  for (int i = 0; i < kWorkersCount; ++i)
  {
    struct WorkerInfo* worker_info = malloc(sizeof(struct WorkerInfo));
    if (worker_info == MALLOC_FAILED)
    {
      snprintf(
        message_buffer,
        kMessageBufferSize,
        "Control block received error: malloc failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, control_block_id);
    }

    worker_info->control_block_id_ = control_block_id;
    worker_info->input_channel_ = channels[i][0];

    int error_code = pthread_create(workers + i, NULL, &WorkerFunction, worker_info);
    if (error_code != kPthreadCreateSuccess)
    {
      snprintf(
        message_buffer,
        kMessageBufferSize,
        "Control block received error: pthread_create failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, control_block_id);
    }
  }

  int input_channel = (int) arg;
  unsigned current_worker_id = 0U;

  while (true)
  {
    int clientfd;
    ssize_t processed_bytes = read(input_channel, &clientfd, sizeof(int));
    if (processed_bytes == kReadFailed)
    {
      snprintf(
        message_buffer,
        kMessageBufferSize,
        "Control block received error: read failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, control_block_id);
    }
    processed_bytes = write(channels[current_worker_id++ & 3][1], &clientfd, sizeof(int));
    if (processed_bytes == kWriteFailed)
    {
      if (errno != EAGAIN)
      {
        snprintf(
          message_buffer,
          kMessageBufferSize,
          "Control block received error: write failed: [%d](%s)",
          errno,
          strerror(errno)
        );
        LOG_FATAL(message_buffer, control_block_id);
      }
      else
      {
        snprintf(
          message_buffer,
          kMessageBufferSize,
          "Control block received error: write failed: [%d](%s)",
          errno,
          strerror(errno)
        );
        LOG_WARNING(message_buffer, control_block_id);
        shutdown(clientfd, SHUT_RDWR);
        close(clientfd);
      }
    }
  }

  return NULL;
}