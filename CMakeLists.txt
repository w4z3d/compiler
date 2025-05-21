cmake_minimum_required(VERSION 3.28.1)
project(compiler)

set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} -march=native -mtune=native -flto -fomit-frame-pointer"
)
set(CMAKE_C_FLAGS_RELEASE
    "${CMAKE_C_FLAGS_RELEASE} -march=native -mtune=native -flto -fomit-frame-pointer"
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Multithreaded builds Min 8 jobs
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# Add sourcefiles
file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp")

# Add target
add_executable(compiler ${SOURCES})

# Link spdlog
target_link_libraries(compiler PRIVATE)
