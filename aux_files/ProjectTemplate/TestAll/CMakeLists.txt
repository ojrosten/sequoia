cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

target_sources(TestMain PRIVATE)

file(GLOB_RECURSE HeaderFiles ../Tests/TestFramework/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

set(BuildSystem SEQUOIA_ROOT/build_system)
set(SourceDir ../Source)

include(${BuildSystem}/Utilities.cmake)

HEADER_DIRECTORIES(AllSourceDirs ${SourceDir}/*.hpp)
target_include_directories(TestMain PRIVATE ${AllSourceDirs})

FINALIZE(TestMain)

