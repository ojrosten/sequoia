cmake_minimum_required(VERSION 3.19)

project(SequioaTestFrameworkHarness DESCRIPTION "End-to-end testing of the Sequoia testing framework"
                                    LANGUAGES CXX)

add_executable(TestFrameworkHarness TestFrameworkHarnessMain.cpp)

target_sources(TestFrameworkHarness PRIVATE ../Tests/TestFramework/TestRunnerEndToEndFreeTest.cpp)

SET(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE_SEQUOIA(TestFrameworkHarness)
