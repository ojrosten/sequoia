cmake_minimum_required(VERSION 3.20)

set(SourceList)

add_library(GeneratedProject STATIC ${SourceList})

target_include_directories(GeneratedProject PUBLIC .)

file(GLOB_RECURSE HeaderFiles *.hpp)
target_sources(GeneratedProject PRIVATE ${HeaderFiles})

target_compile_features(GeneratedProject PUBLIC cxx_std_23)
