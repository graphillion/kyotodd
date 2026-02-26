cmake_minimum_required(VERSION 3.14)
project(KyotoDD CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kyotodd src/main.cpp src/bdd.cpp src/bdd_class.cpp src/bdd_io.cpp src/bdd_ops.cpp)
target_include_directories(kyotodd PRIVATE include)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(kyotodd_test test/test_bdd.cpp src/bdd.cpp src/bdd_class.cpp src/bdd_io.cpp src/bdd_ops.cpp)
target_include_directories(kyotodd_test PRIVATE include)
target_link_libraries(kyotodd_test GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(kyotodd_test)
