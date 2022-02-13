cmake_minimum_required(VERSION 3.20)

set(SourceList)

add_library(AnotherGeneratedProject STATIC ${SourceList})

target_include_directories(AnotherGeneratedProject PUBLIC .)

set(BuildSystem ../../FakeProject/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_finalize_library(AnotherGeneratedProject)
