cmake_minimum_required(VERSION 3.20)

set(BuildSystem ../../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList)

add_library(GeneratedProject STATIC ${SourceList})

target_include_directories(GeneratedProject PUBLIC ../)

sequoia_finalize_library(GeneratedProject)
