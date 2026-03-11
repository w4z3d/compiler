# tests/CMakeLists.txt

# Simple approach: each .c0 file is a test
# The test runs your compiler and checks the exit code

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.c0")

foreach(test_file ${TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(
        NAME ${test_name}
        COMMAND $<TARGET_FILE:cc0> ${test_file} -v -o ${test_file}_out
    )
endforeach()
