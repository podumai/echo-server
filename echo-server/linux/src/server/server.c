#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sync_server/errors/errors.h>
#include <sync_server/logger/logger.h>
#include <sync_server/server/server.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MALLOC_FAILED NULL

const int kServerSocketInitFailed = -1;
const int kSocketRegistryFailed = -1;

static const int kSocketPendingConnections = 5;
static const int kSocketBufferAllocFailed = -1;
static const int kConnectionTimeQuota = 3;
static const int kServerBasePort = 10000;
static const int kSocketBufferSize = 1024;
static const unsigned kWorkersCount = 4U;
static const int kDefaultTimerFlags = 0;
static const int kDefaultSocketProtocol = 0;
static const int kSemPostFailed = -1;
static const int kMessageBufferSize = 256;
static const int kPthreadCreateSuccess = 0;

extern sem_t control_semaphore;

// clang-format off
__attribute__((nonnull(1))) __attribute__((warn_unused_result))
static int CreateSocket(
  struct sockaddr_in* sock_info
)  // clang-format on
{
  int sockfd = socket(AF_INET, SOCK_STREAM, kDefaultSocketProtocol);
  if (sockfd == kSocketFailed)
  {
    return kSocketFailed;
  }
  int error_code = bind(sockfd, (struct sockaddr*) sock_info, sizeof(struct sockaddr_in));
  if (error_code == kBindFailed)
  {
    return kBindFailed;
  }
  error_code = listen(sockfd, kSocketPendingConnections);
  if (error_code == kListenFailed)
  {
    return kListenFailed;
  }
  return sockfd;
}

int InitializeServerSockets(
  struct Server* server
)
{
  LOG_DEBUG("InitializeServerSockets[1]: start sockets initialization", gettid());

  struct sockaddr_in server_addr;
  server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  server_addr.sin_family = AF_INET;

  for (int i = 0; i < SERVER_SOCKETS_COUNT; ++i)
  {
    server_addr.sin_port = htons(kServerBasePort + i);
    server->sockets_[i] = CreateSocket(&server_addr);
    if (server->sockets_[i] == kSocketFailed)
    {
      return kSocketFailed;
    }
  }

  LOG_DEBUG("InitializeServerSockets[2]: end sockets initialization", gettid());
  return 0;
}

int RegisterServerSockets(
  int epfd,  //
  struct Server* server
)
{
  LOG_DEBUG("RegisterServerSockets[1]: start sockets registration", gettid());

  struct epoll_event ev;
  ev.events = EPOLLIN;
  for (int i = 0; i < SERVER_SOCKETS_COUNT; ++i)
  {
    ev.data.fd = server->sockets_[i];
    int error_code = epoll_ctl(epfd, EPOLL_CTL_ADD, server->sockets_[i], &ev);
    if (error_code == kEpollCtlFailed)
    {
      return kEpollCtlFailed;
    }
  }

  LOG_DEBUG("RegisterServerSockets[2]: end sockets registration", gettid());
  return 0;
}

void PrintServerInitInfo(
  struct Server* server
)
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
  puts(
    "\tsocket type: SOCK_STREAM\n"
    "\tprotocol: TCP/IP"
  );
}

int ConfigureClientSocket(
  int clientfd
)
{
  LOG_DEBUG("ConfigureClientSocket[1]: start configurating flags", gettid());

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

  LOG_DEBUG("ConfigureClientSocket[2]: end configurating flags", gettid());
  return 0;
}

struct WorkerInfo
{
  pthread_t control_block_id_;
  int input_channel_;
};

static __thread char message_buffer[kMessageBufferSize];
static const int kWorkerBufferSize = 16;

// clang-format off
__attribute__((nonnull(1)))
static void* WorkerFunction(
  void* arg
)  // clang-format on
{
  int error_code;
  unsigned char buffer[kWorkerBufferSize];
  struct WorkerInfo* worker_info = (struct WorkerInfo*) arg;
  pid_t worker_id = gettid();
  int clientfd;

  timer_t connection_timer;
  struct sigevent se;
  se.sigev_notify = SIGEV_THREAD_ID;
  se.sigev_signo = SIGUSR1;
  se.sigev_value.sival_ptr = &clientfd;

#if defined(sigev_notify_thread_id)
  #pragma push_macro("sigev_notify_thread_id")
  #undef sigev_notify_thread_id
  #define SIGEV_NOTIFY_THREAD_ID_DEFINED
#endif
#define sigev_notify_thread_id _sigev_un._tid

  se.sigev_notify_thread_id = worker_id;

#undef sigev_notify_thread_id
#if defined(SIGEV_NOTIFY_THREAD_ID_DEFINED)
  #pragma pop_macro("sigev_notify_thread_id")
  #undef SIGEV_NOTIFY_THREAD_ID_DEFINED
#endif

  error_code = timer_create(CLOCK_REALTIME, &se, &connection_timer);
  if (error_code == kTimerCreateFailed)
  {
    exit(EXIT_FAILURE);
  }

  struct itimerspec connection_limit = {
    .it_interval.tv_nsec = 0,  //
    .it_interval.tv_sec = 0,
    .it_value.tv_nsec = 0,
    .it_value.tv_sec = kConnectionTimeQuota
  };
  struct itimerspec empty_time = {{0, 0}, {0, 0}};

  while (true)
  {
  NEXT_CLIENT:
    ssize_t processed_bytes = read(worker_info->input_channel_, &clientfd, sizeof(int));
    if (processed_bytes == kReadFailed)
    {
      snprintf(
        message_buffer,  //
        kMessageBufferSize,
        "Worker received error: read[1] failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, worker_id);
    }

    error_code = timer_settime(connection_timer, kDefaultTimerFlags, &connection_limit, NULL);
    if (error_code == kTimerSettimeFailed)
    {
      snprintf(
        message_buffer,  //
        kMessageBufferSize,
        "Worker received error: timer_settime[1] failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, worker_id);
    }

    processed_bytes = 0;
    while (processed_bytes != kWorkerBufferSize)
    {
      ssize_t bytes = read(clientfd, buffer, kWorkerBufferSize - processed_bytes);
      if (bytes == kReadFailed)
      {
        if (errno == EINTR)
        {
          goto NEXT_CLIENT;
        }
        snprintf(
          message_buffer,  //
          kMessageBufferSize,
          "Worker received error: read[2] failed: [%d](%s)",
          errno,
          strerror(errno)
        );
        LOG_FATAL(message_buffer, worker_id);
      }
      else if (bytes == 0)
      {
        break;
      }

      processed_bytes += bytes;
      bytes = write(clientfd, buffer, bytes);
      if (bytes == kWriteFailed)
      {
        if (errno == EINTR)
        {
          goto NEXT_CLIENT;
        }
        snprintf(
          message_buffer,  //
          kMessageBufferSize,
          "Worker received error: write failed: [%d](%s)",
          errno,
          strerror(errno)
        );
        LOG_FATAL(message_buffer, worker_id);
      }
    }

    error_code = timer_settime(connection_timer, 0, &empty_time, NULL);
    if (error_code == kTimerSettimeFailed)
    {
      exit(EXIT_FAILURE);
    }

    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);

    if (errno)
    {
      errno = 0;
      continue;
    }
    snprintf(
      message_buffer,  //
      kMessageBufferSize,
      "Worker: client qouta exceded. [%d] bytes processed.",
      kWorkerBufferSize
    );
    LOG_INFO(message_buffer, worker_id);
  }

  return NULL;
}

void* ControlBlockFunction(
  void* arg
)
{
  pid_t control_block_id = gettid();
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
        message_buffer,  //
        kMessageBufferSize,
        "Control block received error: pthread_create failed: [%d](%s)",
        errno,
        strerror(errno)
      );
      LOG_FATAL(message_buffer, control_block_id);
    }
  }

  int error_code = sem_post(&control_semaphore);
  if (error_code == kSemPostFailed)
  {
    snprintf(
      message_buffer,  //
      kMessageBufferSize,
      "Control block received error: sem_post failed: [%d](%s)",
      errno,
      strerror(errno)
    );
    LOG_FATAL(message_buffer, control_block_id);
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
        message_buffer,  //
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
      snprintf(
        message_buffer,  //
        kMessageBufferSize,
        "Control block received error: write failed: [%d](%s)",
        errno,
        strerror(errno)
      );

      if (errno != EAGAIN)
      {
        LOG_FATAL(message_buffer, control_block_id);
      }
      else
      {
        LOG_WARNING(message_buffer, control_block_id);
        shutdown(clientfd, SHUT_RDWR);
        close(clientfd);
      }
    }
  }

  return NULL;
}