cmake_minimum_required(VERSION 4.0)
project(compiler)

set(CMAKE_CXX_STANDDARD 26)
set(CMAKE_CXX_STANDDARD_REQUIRED ON)

# Build types
if(NOT CMAKE_BUILD_TYPE)
  set(default_build_type "Debug")
  message(
    STATUS
      "Set the build type to `${default_build_type}` as none was specified.")
  set(CMAKE_BUILD_TYPE
      ${default_build_type}
      CACHE STRING "Choose the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "MinSizeRel" "RelWithDebInfo")
endif()
message(STATUS "compiler Build Type: ${CMAKE_BUILD_TYPE}")

# Set the version for myProject
set(myProject_Version_Major 0)
set(myProject_Version_Minor 1)
set(myProject_Version_Patch 0)
set(myProject_Version_Status "-dev")
set(PROJECT_VERSION
    "${myProject_Version_Major}.${myProject_Version_Minor}.${myProject_Version_Patch}${myProject_Version_Status}"
)
message(STATUS "${PROJECT_NAME} version: ${PROJECT_VERSION}")

# Multithreaded builds Min 8 jobs
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# Add sourcefiles
file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp")

# Add target
add_executable(compiler ${SOURCES})

# Add spdlog
add_subdirectory(lib/spdlog)

# Link spdlog
target_link_libraries(compiler PRIVATE spdlog::spdlog_header_only)
