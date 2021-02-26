cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

target_sources(TestMain PRIVATE)

SET(BUILD_SYS /Users/Claire/Dropbox/Sequoia/build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE(TestMain)

