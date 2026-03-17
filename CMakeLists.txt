cmake_minimum_required(VERSION 3.14)
project(KyotoDD CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KYOTODD_LIB_SOURCES
  src/bdd_base.cpp
  src/bdd_class.cpp
  src/bdd_io.cpp
  src/bdd_ops.cpp
  src/zdd_ops.cpp
  src/zdd_adv.cpp
  src/zdd_adv2.cpp
)

add_library(kyotodd_lib STATIC ${KYOTODD_LIB_SOURCES})
target_include_directories(kyotodd_lib PUBLIC include)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(kyotodd_lib PRIVATE -Wall -Wextra)
endif()

add_executable(kyotodd src/main.cpp)
target_link_libraries(kyotodd kyotodd_lib)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(kyotodd_test test/test_bdd.cpp)
target_link_libraries(kyotodd_test kyotodd_lib GTest::gtest_main)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(kyotodd_test PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(kyotodd_test)
