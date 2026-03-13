cmake_minimum_required(VERSION 3.28.1)
project(cc0)
enable_testing()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Multithreaded builds Min 8 jobs
set(CMAKE_BUILD_PARALLEL_LEVEL 8)

# Add sourcefiles
file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp")
# Add target
add_executable(cc0 ${SOURCES})


add_subdirectory(tests)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/runtime.o
    COMMAND clang -c -O2
        ${CMAKE_SOURCE_DIR}/runtime/runtime.c
        -o ${CMAKE_BINARY_DIR}/runtime.o
    DEPENDS ${CMAKE_SOURCE_DIR}/runtime/runtime.c
    COMMENT "Compiling runtime"
)

add_custom_target(runtime ALL
    DEPENDS ${CMAKE_BINARY_DIR}/runtime.o
)

add_dependencies(cc0 runtime)
