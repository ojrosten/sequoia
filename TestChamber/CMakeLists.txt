cmake_minimum_required(VERSION 3.20)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

include(../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir ../Tests)

target_sources(TestChamber PRIVATE
               ${TestDir}/Algorithms/AlgorithmsTest.cpp
)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental)
