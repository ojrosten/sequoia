cmake_minimum_required(VERSION 3.20)

set(BuildSystem ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList
    Global/Stuff/Defs.cpp
    Global/Stuff/Global.cpp
    Maths/Angle.cpp
    Maths/Probability.cpp
    Stuff/Doohicky.cpp
    Stuff/Widget.cpp
    Weather/Cloud.cpp
    hominins/Human.cpp)

add_library(MyProject STATIC ${SourceList})

target_include_directories(MyProject PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(MyProject)
