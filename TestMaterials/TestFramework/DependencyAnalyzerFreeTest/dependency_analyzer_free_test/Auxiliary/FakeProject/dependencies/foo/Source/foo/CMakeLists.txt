cmake_minimum_required(VERSION 4.2)

add_library(foo STATIC Utilities/Helper.cpp)

target_include_directories(foo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)
