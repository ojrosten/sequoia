cmake_minimum_required(VERSION 3.19)

project(MyProjectTests LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem /build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

#!add_subdirectory(${SourceDir} MyProject)
#!target_link_libraries(TestMain PUBLIC MyProject)

target_sources(TestMain PRIVATE)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestMain PRIVATE ${SourceDir})
target_include_directories(TestMain PRIVATE ${TestDir})

FINALIZE(TestMain)

