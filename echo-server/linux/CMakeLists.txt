set(BASE_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(LINUX_SERVER)
set(linux_server_headers)
add_executable(LINUX_SERVER)
target_sources(
  LINUX_SERVER
    PRIVATE
      FILE_SET linux_server_headers
      TYPE HEADERS
      BASE_DIRS
        "${BASE_INCLUDE_DIR}"
      FILES
        "${BASE_INCLUDE_DIR}/sync_server/logger/logger.h"
        "${BASE_INCLUDE_DIR}/sync_server/errors/errors.h"
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
)
target_compile_options(
  LINUX_SERVER
    PUBLIC
      "-std=gnu11"
)
set_target_properties(
  LINUX_SERVER
    PROPERTIES
      OUTPUT_NAME
        "server$<$<PLATFORM_ID:Windows>:.exe>"
      RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

set(LINUX_SERVER_LOGGER)
set(linux_server_logger_headers)
add_library(LINUX_SERVER_LOGGER)
target_sources(
  LINUX_SERVER_LOGGER
    PRIVATE
      FILE_SET linux_server_logger_headers
      TYPE HEADERS
      BASE_DIRS
        "${BASE_INCLUDE_DIR}"
      FILES
        "${BASE_INCLUDE_DIR}/sync_server/logger/logger.h"
        "${BASE_INCLUDE_DIR}/sync_server/errors/errors.h"
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/logger/logger.c"
)
target_compile_options(
  LINUX_SERVER_LOGGER
    PUBLIC
      "-std=gnu11"
)
set_target_properties(
  LINUX_SERVER_LOGGER
    PROPERTIES
      OUTPUT_NAME
        "logger"
      RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/lib"
)

set(LINUX_SERVER_LIB)
set(linux_server_lib_headers)
add_library(LINUX_SERVER_LIB)
target_sources(
  LINUX_SERVER_LIB
    PRIVATE
      FILE_SET linux_server_lib_headers
      TYPE HEADERS
      BASE_DIRS
        "${BASE_INCLUDE_DIR}"
      FILES
        "${BASE_INCLUDE_DIR}/sync_server/logger/logger.h"
        "${BASE_INCLUDE_DIR}/sync_server/errors/errors.h"
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/server/server.c"
)
target_compile_options(
  LINUX_SERVER_LIB
    PUBLIC
      "-std=gnu11"
)
set_target_properties(
  LINUX_SERVER_LIB
    PROPERTIES
      OUTPUT_NAME
        "linserver"
      RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/lib"
)
target_link_libraries(
  LINUX_SERVER_LIB
    PRIVATE
      LINUX_SERVER_LOGGER
)

target_link_libraries(
  LINUX_SERVER
    PRIVATE
      LINUX_SERVER_LOGGER
      LINUX_SERVER_LIB
)