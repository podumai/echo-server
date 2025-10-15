#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sync_server/logger/logger.h>
#include <time.h>

static pthread_mutex_t logger_m = PTHREAD_MUTEX_INITIALIZER;
static const int kLoggerBufferSize = 256;

__attribute__((nonnull(1, 2))) //
void CreateLog(
  const char* level,  //
  const char* message,
  unsigned long id
)
{
  static __thread char buffer[kLoggerBufferSize];

  time_t current_time = time(NULL);
  struct tm broken_time;
  localtime_r(&current_time, &broken_time);
  strftime(buffer, kLoggerBufferSize, "%F/%H:%M:%S", &broken_time);
  printf("[LOG][%s][%s] ID:%lu |\n%s\n", level, buffer, id, message);

  if (level[0] == 'F')
  {
    fputs("[MESSAGE][FATAL] Logger received fatal status.\nExiting...\n", stderr);
    exit(EXIT_FAILURE);
  }
}