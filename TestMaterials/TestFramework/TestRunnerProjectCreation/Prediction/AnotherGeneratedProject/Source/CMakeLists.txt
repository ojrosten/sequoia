cmake_minimum_required(VERSION 3.20)

set(SourceList)

add_library(AnotherGeneratedProject STATIC ${SourceList})

target_include_directories(AnotherGeneratedProject PUBLIC .)

file(GLOB_RECURSE HeaderFiles *.h*)
target_sources(AnotherGeneratedProject PRIVATE ${HeaderFiles})

target_compile_features(AnotherGeneratedProject PUBLIC cxx_std_23)
