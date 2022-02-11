cmake_minimum_required(VERSION 3.20)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

find_package(Threads REQUIRED)

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir "../Tests")

target_sources(TestChamber PRIVATE ${TestDir}/Experimental/ExperimentalTest.cpp)

set(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE_SEQUOIA(TestChamber)
