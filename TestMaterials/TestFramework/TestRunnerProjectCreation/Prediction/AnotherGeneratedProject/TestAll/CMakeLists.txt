cmake_minimum_required(VERSION 3.20)

project(AnotherGeneratedProjectTests LANGUAGES CXX)

set(BuildSystem ../dependencies/sequoia/build_system)
set(TestDir ../Tests)
set(SourceDir ../Source)

include(${BuildSystem}/Utilities.cmake)

sequoia_init()

add_executable(TestAll TestAllMain.cpp)

#!add_subdirectory(${SourceDir} AnotherGeneratedProject)
#!target_link_libraries(TestAll PUBLIC AnotherGeneratedProject)

target_sources(TestAll PRIVATE)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

sequoia_finalize(TestAll ${TestDir} Tests)

