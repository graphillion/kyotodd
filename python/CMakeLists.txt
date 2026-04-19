cmake_minimum_required(VERSION 3.15)
project(kyotodd_python CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(pybind11 REQUIRED)

# Path to the KyotoDD C++ sources
set(KYOTODD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
    "Path to KyotoDD root directory")

set(KYOTODD_SOURCES
    ${KYOTODD_ROOT}/src/bdd_base.cpp
    ${KYOTODD_ROOT}/src/bdd_class.cpp
    ${KYOTODD_ROOT}/src/bdd_class_iter.cpp
    ${KYOTODD_ROOT}/src/bdd_io.cpp
    ${KYOTODD_ROOT}/src/bdd_ops.cpp
    ${KYOTODD_ROOT}/src/bdd_ops_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_ops.cpp
    ${KYOTODD_ROOT}/src/zdd_ops_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_filter.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_filter_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_count.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_count_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_weight.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_weight_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_rank.cpp
    ${KYOTODD_ROOT}/src/zdd_adv_rank_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_adv2.cpp
    ${KYOTODD_ROOT}/src/zdd_adv2_iter.cpp
    ${KYOTODD_ROOT}/src/qdd.cpp
    ${KYOTODD_ROOT}/src/qdd_iter.cpp
    ${KYOTODD_ROOT}/src/unreduced_dd.cpp
    ${KYOTODD_ROOT}/src/unreduced_dd_iter.cpp
    ${KYOTODD_ROOT}/src/pidd.cpp
    ${KYOTODD_ROOT}/src/rotpidd.cpp
    ${KYOTODD_ROOT}/src/seqbdd.cpp
    ${KYOTODD_ROOT}/src/zdd_weight_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_rank_iter.cpp
    ${KYOTODD_ROOT}/src/zdd_random_iter.cpp
    ${KYOTODD_ROOT}/src/mvdd.cpp
    ${KYOTODD_ROOT}/src/mtbdd.cpp
    ${KYOTODD_ROOT}/src/mtbdd_iter.cpp
    ${KYOTODD_ROOT}/src/svg_export.cpp
)

pybind11_add_module(_core
    src/kyotodd/_binding.cpp
    ${KYOTODD_SOURCES}
)

target_include_directories(_core PRIVATE ${KYOTODD_ROOT}/include)

install(TARGETS _core DESTINATION kyotodd)
