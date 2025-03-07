cmake_minimum_required(VERSION 3.0)

project(TEST CXX)

set(CMAKE_CXX_STANDARD 11)							##设置C++标准
SET(CMAEK_CXX_STANDARD_REQUIRED ON)

add_executable(test test.cpp MemoryPool.cpp)