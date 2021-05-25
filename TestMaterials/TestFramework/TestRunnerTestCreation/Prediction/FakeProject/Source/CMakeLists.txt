cmake_minimum_required(VERSION 3.19)

set(SourceList
    fakeProject/Global/Stuff/Defs.cpp
    fakeProject/Global/Stuff/Global.cpp
    fakeProject/Stuff/Doohicky.cpp
    fakeProject/Stuff/Widget.cpp)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC .)

file(GLOB_RECURSE HeaderFiles *.hpp)
target_sources(MyProject PRIVATE ${HeaderFiles})

target_compile_features(MyProject PUBLIC cxx_std_20)
