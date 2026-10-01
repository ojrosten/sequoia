cmake_minimum_required(VERSION 4.2)

set(BuildSystem ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/sequoia/build_system)
include(${BuildSystem}/Utilities.cmake)

sequoia_init()

set(SourceList
    Maths/Helper.cpp
    Maths/Probability.cpp
    Stuff/Foo.cpp
    Stuff/FooDefinitions.cpp
    Stuff/Substitutions.cpp
    Stuff/Unrelated.cpp
    Utilities/Thing/UniqueThing.cpp
    Utilities/UsefulThings.cpp)

add_library(FakeProject STATIC ${SourceList})

target_include_directories(FakeProject PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)
target_link_libraries(FakeProject PUBLIC foo)

sequoia_finalize_library(FakeProject)
