cmake_minimum_required(VERSION 3.14)
project(KyotoDD CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KYOTODD_LIB_SOURCES
  src/bdd_base.cpp
  src/bdd_class.cpp
  src/bdd_io.cpp
  src/bdd_ops.cpp
  src/zdd_ops.cpp
  src/zdd_adv.cpp
)

add_library(kyotodd_lib STATIC ${KYOTODD_LIB_SOURCES})
target_include_directories(kyotodd_lib PUBLIC include)

add_executable(kyotodd src/main.cpp)
target_link_libraries(kyotodd kyotodd_lib)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(kyotodd_test test/test_bdd.cpp)
target_link_libraries(kyotodd_test kyotodd_lib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(kyotodd_test)
