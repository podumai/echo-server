add_subdirectory(beast)

if(BUILD_LINUX_IMPL)
  message(STATUS "Linux implementation build: selected")
  add_subdirectory(linux)
else()
  message(STATUS "Linux implementation build: skipped")
endif()