cmake_minimum_required(VERSION 3.10)
project(KyotoDD CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kyotodd src/main.cpp)
target_include_directories(kyotodd PRIVATE include)
