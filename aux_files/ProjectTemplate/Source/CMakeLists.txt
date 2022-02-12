cmake_minimum_required(VERSION 3.20)

set(SourceList)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC .)

file(GLOB_RECURSE HeaderFiles *.hpp)
target_sources(MyProject PRIVATE ${HeaderFiles})

target_compile_features(MyProject PUBLIC cxx_std_23)
