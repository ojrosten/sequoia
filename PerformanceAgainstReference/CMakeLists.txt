cmake_minimum_required(VERSION 4.2)

project(SequoiaPerformanceAgainstReference DESCRIPTION "The Sequoia Library's performance against a reference tree's"
                                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(PerformanceAgainstReference PerformanceAgainstReferenceMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(PerformanceAgainstReference PRIVATE ${TestDir}/TestFramework/RunnerPerformanceAgainstReferenceTest.cpp)

sequoia_finalize_self(PerformanceAgainstReference ${TestDir} Tests)
