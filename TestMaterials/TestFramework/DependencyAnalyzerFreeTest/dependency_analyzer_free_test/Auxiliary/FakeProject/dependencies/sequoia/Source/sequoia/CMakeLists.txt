cmake_minimum_required(VERSION 4.2)

include(${CMAKE_CURRENT_LIST_DIR}/../../build_system/Utilities.cmake)

sequoia_init()

set(SourceList
    TestFramework/FileEditors.cpp)

add_library(sequoia STATIC ${SourceList})

target_include_directories(sequoia PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(sequoia)
