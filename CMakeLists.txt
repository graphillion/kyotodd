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
  src/pidd.cpp
  src/rotpidd.cpp
  src/seqbdd.cpp
)

add_library(kyotodd_lib STATIC ${KYOTODD_LIB_SOURCES})
target_include_directories(kyotodd_lib PUBLIC include)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(kyotodd_lib PRIVATE -Wall -Wextra)
endif()

add_executable(kyotodd src/main.cpp)
target_link_libraries(kyotodd kyotodd_lib)

add_executable(bddqueen app/BDDQueen/bddqueen.cpp)
target_link_libraries(bddqueen kyotodd_lib)

add_executable(bddqueen_BDD app/BDDQueen/bddqueen_BDD.cpp)
target_link_libraries(bddqueen_BDD kyotodd_lib)

add_executable(bddqueen_ZDD app/BDDQueen/bddqueen_ZDD.cpp)
target_link_libraries(bddqueen_ZDD kyotodd_lib)

option(KYOTODD_BUILD_TESTS "Build tests (requires GoogleTest via FetchContent)" ON)

if(KYOTODD_BUILD_TESTS)
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

  add_executable(pidd_test test/test_pidd.cpp)
  target_link_libraries(pidd_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pidd_test PRIVATE -Wall -Wextra)
  endif()

  add_executable(rotpidd_test test/test_rotpidd.cpp)
  target_link_libraries(rotpidd_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(rotpidd_test PRIVATE -Wall -Wextra)
  endif()

  add_executable(seqbdd_test test/test_seqbdd.cpp)
  target_link_libraries(seqbdd_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(seqbdd_test PRIVATE -Wall -Wextra)
  endif()

  add_executable(ddbase_test test/test_ddbase.cpp)
  target_link_libraries(ddbase_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ddbase_test PRIVATE -Wall -Wextra)
  endif()

  include(GoogleTest)
  gtest_discover_tests(kyotodd_test)
  gtest_discover_tests(pidd_test)
  gtest_discover_tests(rotpidd_test)
  gtest_discover_tests(seqbdd_test)
  gtest_discover_tests(ddbase_test)
endif()
