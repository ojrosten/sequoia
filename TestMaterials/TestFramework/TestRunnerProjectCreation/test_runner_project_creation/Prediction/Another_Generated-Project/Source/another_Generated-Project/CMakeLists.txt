cmake_minimum_required(VERSION 3.20)

set(BuildSystem ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList)

add_library(Another_Generated-Project STATIC ${SourceList})

target_include_directories(Another_Generated-Project PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(Another_Generated-Project)
