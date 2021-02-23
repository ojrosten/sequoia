cmake_minimum_required(VERSION 3.19)

project(My Project LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

SET(BUILD_SYS SEQUOIA_ROOT/build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE(TestMain)

