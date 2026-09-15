cmake_minimum_required(VERSION 3.20)

set(BuildSystem ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(MyProject)
