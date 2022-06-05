cmake_minimum_required(VERSION 3.20)

set(BuildSystem ../build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList
    fakeProject/Global/Stuff/Defs.cpp
    fakeProject/Global/Stuff/Global.cpp
    fakeProject/Maths/Angle.cpp
    fakeProject/Maths/Probability.cpp
    fakeProject/Stuff/Doohicky.cpp
    fakeProject/Stuff/Widget.cpp
    fakeProject/hominins/Human.cpp)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC .)

sequoia_finalize_library(MyProject)
