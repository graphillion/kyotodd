cmake_minimum_required(VERSION 3.15)
project(kyotodd_python CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 REQUIRED)

# Path to the KyotoDD C++ sources
set(KYOTODD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
    "Path to KyotoDD root directory")

set(KYOTODD_SOURCES
    ${KYOTODD_ROOT}/src/bdd_base.cpp
    ${KYOTODD_ROOT}/src/bdd_class.cpp
    ${KYOTODD_ROOT}/src/bdd_io.cpp
    ${KYOTODD_ROOT}/src/bdd_ops.cpp
    ${KYOTODD_ROOT}/src/zdd_ops.cpp
    ${KYOTODD_ROOT}/src/zdd_adv.cpp
    ${KYOTODD_ROOT}/src/zdd_adv2.cpp
    ${KYOTODD_ROOT}/src/pidd.cpp
    ${KYOTODD_ROOT}/src/rotpidd.cpp
)

pybind11_add_module(_core
    src/kyotodd/_binding.cpp
    ${KYOTODD_SOURCES}
)

target_include_directories(_core PRIVATE ${KYOTODD_ROOT}/include)

install(TARGETS _core DESTINATION kyotodd)
