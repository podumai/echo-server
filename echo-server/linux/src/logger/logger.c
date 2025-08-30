#include <sync_server/logger/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

static pthread_mutex_t logger_m = PTHREAD_MUTEX_INITIALIZER;
static const int kLoggerBufferSize = 256;
static const int kMutexLockSuccess = 0;
static const int kMutexUnlockSuccess = 0;

__attribute__((nonnull(1, 2)))
void CreateLog(
  const char* level, //
  const char* message,
  unsigned long id
)
{
  static char buffer[kLoggerBufferSize];

  int error_code = pthread_mutex_lock(&logger_m);
  if (error_code != kMutexLockSuccess)
  {
    exit(EXIT_FAILURE);
  }

  time_t current_time = time(NULL);
  struct tm* broken_time = localtime(&current_time);
  strftime(buffer, kLoggerBufferSize, "%F/%H:%M:%S", broken_time);
  printf("[LOG][%s][%s] ID:%lu |\n%s\n", level, buffer, id, message);
  if (level[0] == 'F')
  {
    fputs("[MESSAGE][FATAL] Logger received fatal status.\nExiting...\n", stderr);
    exit(EXIT_FAILURE);
  }

  error_code = pthread_mutex_unlock(&logger_m);
  if (error_code != kMutexUnlockSuccess)
  {
    exit(EXIT_FAILURE);
  }
}