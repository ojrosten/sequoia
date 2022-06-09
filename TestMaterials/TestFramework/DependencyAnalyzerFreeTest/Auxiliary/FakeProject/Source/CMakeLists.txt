cmake_minimum_required(VERSION 3.19)

set(SourceList
    generatedProject/Maths/Probability.cpp
    generatedProject/Utilities/Thing/UniqueThing.cpp
    generatedProject/Utilities/UsefulThings.cpp)

add_library(GeneratedProject STATIC ${SourceList})

target_include_directories(GeneratedProject PUBLIC .)

file(GLOB_RECURSE HeaderFiles *.hpp)
target_sources(GeneratedProject PRIVATE ${HeaderFiles})

target_compile_features(GeneratedProject PUBLIC cxx_std_20)
