#pragma once

#include <netinet/in.h>

#define SERVER_SOCKETS_COUNT 10

struct Server
{
  int sockets_[SERVER_SOCKETS_COUNT];
  struct sockaddr_in info_;
};

__attribute__((nonnull(1))) __attribute__((warn_unused_result))
extern int InitializeServerSockets(struct Server* server);

__attribute__((nonnull(2))) __attribute__((warn_unused_result))
extern int RegisterServerSockets(
  int epfd,  //
  struct Server* server
);

__attribute__((nonnull(1)))
extern void PrintServerInitInfo(struct Server* server);

__attribute__((warn_unused_result))
extern int ConfigureClientSocket(int clientfd);

__attribute__((nonnull(1)))
extern void* ControlBlockFunction(void* arg);