cmake_minimum_required(VERSION 3.20)

project(Another_Generated-ProjectTests LANGUAGES CXX)

set(Dependencies ${CMAKE_CURRENT_LIST_DIR}/../dependencies)
set(BuildSystem ${Dependencies}/sequoia/build_system)
set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)
set(SourceDir ${CMAKE_CURRENT_LIST_DIR}/../Source)

include(${BuildSystem}/Utilities.cmake)

sequoia_init()

add_executable(TestAll TestAllMain.cpp)

#!add_subdirectory(${SourceDir}/another_Generated-Project Another_Generated-Project)
#!target_link_libraries(TestAll PUBLIC Another_Generated-Project)

target_sources(TestAll PRIVATE)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

add_subdirectory(${Dependencies}/sequoia/Source/sequoia sequoia)
target_link_libraries(TestAll PRIVATE sequoia)

sequoia_finalize_tests(TestAll ${TestDir} Tests)

