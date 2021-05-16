cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem SEQUOIA_ROOT/build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

target_sources(TestMain PRIVATE)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestMain PRIVATE ${SourceDir})
target_include_directories(TestMain PRIVATE ${TestDir})

FINALIZE(TestMain)

