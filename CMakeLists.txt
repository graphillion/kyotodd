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
  src/bdd_ops_iter.cpp
  src/zdd_ops.cpp
  src/zdd_ops_iter.cpp
  src/zdd_adv_filter.cpp
  src/zdd_adv_filter_iter.cpp
  src/zdd_adv_count.cpp
  src/zdd_adv_weight.cpp
  src/zdd_adv_rank.cpp
  src/zdd_adv2.cpp
  src/pidd.cpp
  src/rotpidd.cpp
  src/seqbdd.cpp
  src/unreduced_dd.cpp
  src/qdd.cpp
  src/bddv.cpp
  src/zbddv.cpp
  src/sop.cpp
  src/btoi.cpp
  src/ctoi.cpp
  src/mlzbddv.cpp
  src/zdd_weight_iter.cpp
  src/zdd_rank_iter.cpp
  src/zdd_random_iter.cpp
  src/mtbdd.cpp
  src/mvdd.cpp
  src/svg_export.cpp
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

add_executable(svg_examples app/SvgExamples/svg_examples.cpp)
target_link_libraries(svg_examples kyotodd_lib)

option(KYOTODD_BUILD_TESTS "Build tests (requires GoogleTest via FetchContent)" ON)

if(KYOTODD_BUILD_TESTS)
  # Google Test
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz
    URL_HASH SHA256=81964fe578e9bd7c94dfdb09c8e4d6e6759e19967e397dbea48d1c10e45d0df2
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(bdd1_test test/test_bdd1.cpp)
  target_link_libraries(bdd1_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bdd1_test PRIVATE -Wall -Wextra)
  endif()

  add_executable(bdd2_test test/test_bdd2.cpp)
  target_link_libraries(bdd2_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bdd2_test PRIVATE -Wall -Wextra)
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

  add_executable(unreduced_dd_test test/test_unreduced_dd.cpp)
  target_link_libraries(unreduced_dd_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(unreduced_dd_test PRIVATE -Wall -Wextra)
  endif()

  add_executable(qdd_test test/test_qdd.cpp)
  target_link_libraries(qdd_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(qdd_test PRIVATE -Wall -Wextra)
  endif()

  add_executable(bddv_test test/test_bddv.cpp)
  target_link_libraries(bddv_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bddv_test PRIVATE -Wall -Wextra)
  endif()

  include(GoogleTest)
  gtest_discover_tests(bdd1_test)
  gtest_discover_tests(bdd2_test)
  gtest_discover_tests(pidd_test)
  gtest_discover_tests(rotpidd_test)
  gtest_discover_tests(seqbdd_test)
  gtest_discover_tests(ddbase_test)
  gtest_discover_tests(unreduced_dd_test)
  gtest_discover_tests(qdd_test)
  gtest_discover_tests(bddv_test)

  add_executable(zdd_ops1_test test/test_zdd_ops1.cpp)
  target_link_libraries(zdd_ops1_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_ops1_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_ops1_test)

  add_executable(zdd_ops2_test test/test_zdd_ops2.cpp)
  target_link_libraries(zdd_ops2_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_ops2_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_ops2_test)

  add_executable(zdd_count1_test test/test_zdd_count1.cpp)
  target_link_libraries(zdd_count1_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_count1_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_count1_test)

  add_executable(zdd_count2_test test/test_zdd_count2.cpp)
  target_link_libraries(zdd_count2_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_count2_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_count2_test)

  add_executable(zdd_count3_test test/test_zdd_count3.cpp)
  target_link_libraries(zdd_count3_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_count3_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_count3_test)

  add_executable(bdd_io_test test/test_bdd_io.cpp)
  target_link_libraries(bdd_io_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(bdd_io_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(bdd_io_test)

  add_executable(zdd_advanced1_test test/test_zdd_advanced1.cpp)
  target_link_libraries(zdd_advanced1_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_advanced1_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_advanced1_test)

  add_executable(zdd_advanced2_test test/test_zdd_advanced2.cpp)
  target_link_libraries(zdd_advanced2_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_advanced2_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_advanced2_test)

  add_executable(zdd_advanced3_test test/test_zdd_advanced3.cpp)
  target_link_libraries(zdd_advanced3_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_advanced3_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_advanced3_test)

  add_executable(zbddv_test test/test_zbddv.cpp)
  target_link_libraries(zbddv_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zbddv_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zbddv_test)

  add_executable(sop_test test/test_sop.cpp)
  target_link_libraries(sop_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(sop_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(sop_test)

  add_executable(btoi_test test/test_btoi.cpp)
  target_link_libraries(btoi_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(btoi_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(btoi_test)

  add_executable(ctoi_test test/test_ctoi.cpp)
  target_link_libraries(ctoi_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ctoi_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(ctoi_test)

  add_executable(mlzbddv_test test/test_mlzbddv.cpp)
  target_link_libraries(mlzbddv_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mlzbddv_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(mlzbddv_test)

  add_executable(zdd_weight_iter_test test/test_zdd_weight_iter.cpp)
  target_link_libraries(zdd_weight_iter_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_weight_iter_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_weight_iter_test)

  add_executable(zdd_rank_iter_test test/test_zdd_rank_iter.cpp)
  target_link_libraries(zdd_rank_iter_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_rank_iter_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_rank_iter_test)

  add_executable(zdd_random_iter_test test/test_zdd_random_iter.cpp)
  target_link_libraries(zdd_random_iter_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(zdd_random_iter_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(zdd_random_iter_test)

  add_executable(mtbdd_test test/test_mtbdd.cpp)
  target_link_libraries(mtbdd_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mtbdd_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(mtbdd_test)

  add_executable(mvdd1_test test/test_mvdd1.cpp)
  target_link_libraries(mvdd1_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mvdd1_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(mvdd1_test)

  add_executable(mvdd2_test test/test_mvdd2.cpp)
  target_link_libraries(mvdd2_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mvdd2_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(mvdd2_test)

  add_executable(svg_export_test test/test_svg_export.cpp)
  target_link_libraries(svg_export_test kyotodd_lib GTest::gtest_main)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(svg_export_test PRIVATE -Wall -Wextra)
  endif()
  gtest_discover_tests(svg_export_test)
endif()
