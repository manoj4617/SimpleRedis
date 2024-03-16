# Find any source files with a test-like pattern
file(GLOB_RECURSE TEST_SOURCES *.cpp)

# Create the test executable
add_executable(my_tests ${TEST_SOURCES})

# Link against GoogleTest libraries and your libraries under test
target_link_libraries(my_tests GTest::gtest_main GTest::gtest parser server client) 