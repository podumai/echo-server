#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

__attribute__((nonnull(1, 2)))
extern void CreateLog(
  const char* level, //
  const char* message,
  unsigned long id
);

#define LOG_INFO(message, id) CreateLog("INFO", message, id)
#define LOG_WARNING(message, id) CreateLog("WARNING", message, id)
#define LOG_FATAL(message, id) CreateLog("FATAL", message, id)

#ifndef NDEBUG
  #define LOG_DEBUG(message, id) CreateLog("DEBUG", message, id)
#else
  #define LOG_DEBUG(message, id) do { } while (0)
#endif

#ifdef __cplusplus
}
#endif