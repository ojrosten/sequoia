cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

target_sources(TestMain PRIVATE)

set(BuildSystem SEQUOIA_ROOT/build_system)

include(${BuildSystem}/Utilities.cmake)

file(GLOB_RECURSE HeaderFiles ../Tests/TestFramework/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

FINALIZE(TestMain)

