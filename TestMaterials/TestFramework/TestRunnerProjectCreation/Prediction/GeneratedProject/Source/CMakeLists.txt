cmake_minimum_required(VERSION 3.20)

set(SourceList)

add_library(GeneratedProject STATIC ${SourceList})

target_include_directories(GeneratedProject PUBLIC .)

set(BuildSystem ../../FakeProject/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_finalize_library(GeneratedProject)
