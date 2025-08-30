#pragma once

enum SystemCallError
{
  kUnknown,
  kReadFailed = -1,
  kWriteFailed = -1,
  kPipeFailed = -1,
  kEpollCreateFailed = -1,
  kEpollCtlFailed = -1,
  kEpollWaitFailed = -1,
  kSocketFailed = -1,
  kBindFailed = -1,
  kListenFailed = -1,
  kAcceptFailed = -1,
  kFcntlFailed = -1,
  kTimerCreateFailed = -1,
  kTimerSettimeFailed = -1
};

extern const int kServerSocketInitFailed;
extern const int kSocketRegistryFailed;