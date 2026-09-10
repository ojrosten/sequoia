cmake_minimum_required(VERSION 3.20)

project(GeneratedProjectTests LANGUAGES CXX)

set(Dependencies ${CMAKE_CURRENT_LIST_DIR}/../dependencies)
set(BuildSystem ${Dependencies}/sequoia/build_system)
set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)
set(SourceDir ${CMAKE_CURRENT_LIST_DIR}/../Source)

include(${BuildSystem}/Utilities.cmake)

sequoia_init()

add_executable(TestAll TestAllMain.cpp)

#!add_subdirectory(${SourceDir}/generatedProject GeneratedProject)
#!target_link_libraries(TestAll PUBLIC GeneratedProject)

target_sources(TestAll PRIVATE)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

add_subdirectory(${Dependencies}/sequoia/Source/sequoia sequoia)
target_link_libraries(TestAll PRIVATE sequoia)

sequoia_finalize_tests(TestAll ${TestDir} Tests)

