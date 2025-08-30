message(CHECK_START "Detecting fmt package.")

find_package(fmt QUIET)

if(fmt_FOUND)
  message(CHECK_PASS "found")
  message(STATUS "Server on top of Boost.Beast, Boost.Asio and fmt will be built.")
  set(BEAST_SERVER)
  add_executable(BEAST_SERVER)
  target_sources(
    BEAST_SERVER
      PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/server.cpp"
  )
  target_link_libraries(
    BEAST_SERVER
      PRIVATE
        fmt::fmt
  )
  target_compile_features(
    BEAST_SERVER
      PRIVATE
        cxx_std_23
      PUBLIC
        cxx_std_17
  )
  set_target_properties(
    BEAST_SERVER
      PROPERTIES
        OUTPUT_NAME
          "server$<$<PLATFORM_ID:Windows>:.exe>"
        RUNTIME_OUTPUT_DIRECTORY
          "${CMAKE_CURRENT_BINARY_DIR}/bin"
  )
else()
  message(CHECK_FAIL "not found")
  message(WARNING "Server on top of Boost.Beast, Boost.Asio and fmt will not be built.")
endif()