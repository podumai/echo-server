message(CHECK_START "Detecting fmt package.")

find_package(fmt QUIET)

if(fmt_FOUND)
  message(CHECK_PASS "found")
  message(STATUS "Server on top of Boost.Asio and fmt will be built.")

  set(SERVER_LIB)
  set(server_lib_headers)
  add_library(SERVER_LIB SHARED)
  target_sources(
    SERVER_LIB
      PRIVATE
        FILE_SET server_lib_headers
        TYPE HEADERS
        BASE_DIRS
          "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
          "${CMAKE_CURRENT_SOURCE_DIR}/include/client/session/session.hpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/include/server/server.hpp"
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/client/session/session.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/server/server.cpp"
  )
  target_compile_features(
    SERVER_LIB
      PRIVATE
        cxx_std_23
      PUBLIC
        cxx_std_17
  )
  set_target_properties(
    SERVER_LIB
      PROPERTIES
        OUTPUT_NAME
          "server"
        RUNTIME_OUTPUT_DIRECTORY
          "${CMAKE_CURRENT_BINARY_DIR}/lib"
  )

  set(ASIO_SERVER)
  set(asio_server_headers)
  add_executable(ASIO_SERVER)
  target_sources(
    ASIO_SERVER
      PRIVATE
        FILE_SET asio_server_headers
        TYPE HEADERS
        BASE_DIRS
          "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
          "${CMAKE_CURRENT_SOURCE_DIR}/include/server/server.hpp"
          "${CMAKE_CURRENT_SOURCE_DIR}/include/client/session/session.hpp"
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
  )
  target_link_libraries(
    ASIO_SERVER
      PRIVATE
        fmt::fmt
        SERVER_LIB
  )
  target_compile_features(
    ASIO_SERVER
      PRIVATE
        cxx_std_23
      PUBLIC
        cxx_std_17
  )
  set_target_properties(
    ASIO_SERVER
      PROPERTIES
        OUTPUT_NAME
          "server$<$<PLATFORM_ID:Windows>:.exe>"
        RUNTIME_OUTPUT_DIRECTORY
          "${CMAKE_CURRENT_BINARY_DIR}/bin"
  )
else()
  message(CHECK_FAIL "not found")
  message(WARNING "Server on top of Boost.Asio and fmt will not be built.")
endif()