cmake_minimum_required(VERSION 3.20)

set(SourceList
    fakeProject/Global/Stuff/Defs.cpp
    fakeProject/Global/Stuff/Global.cpp
    fakeProject/Maths/Angle.cpp
    fakeProject/Maths/Probability.cpp
    fakeProject/Stuff/Doohicky.cpp
    fakeProject/Stuff/Widget.cpp)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC .)

set(BuildSystem ../build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_finalize_library(MyProject)
