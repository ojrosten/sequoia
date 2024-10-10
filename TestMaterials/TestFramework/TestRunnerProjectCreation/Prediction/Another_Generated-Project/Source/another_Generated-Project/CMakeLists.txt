cmake_minimum_required(VERSION 3.20)

set(BuildSystem ../../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList)

add_library(Another_Generated-Project STATIC ${SourceList})

target_include_directories(Another_Generated-Project PUBLIC ../)

sequoia_finalize_library(Another_Generated-Project)
