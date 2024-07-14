cmake_minimum_required(VERSION 3.20)

set(BuildSystem ../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList)

add_library(AnotherGeneratedProject STATIC ${SourceList})

target_include_directories(AnotherGeneratedProject PUBLIC .)

sequoia_finalize_library(AnotherGeneratedProject)
