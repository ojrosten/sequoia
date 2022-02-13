cmake_minimum_required(VERSION 3.20)

set(SourceList)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC .)

set(BuildSystem ../build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_finalize_library(MyProject)
