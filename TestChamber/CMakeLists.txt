cmake_minimum_required(VERSION 3.19)

project(SequioaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

add_executable(TestChamber TestChamberMain.cpp)

target_sources(TestChamber PRIVATE)

SET(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE(TestChamber)
