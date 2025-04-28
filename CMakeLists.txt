cmake_minimum_required(VERSION 3.31.6)
project(compiler)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
