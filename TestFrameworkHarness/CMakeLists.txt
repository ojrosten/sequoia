cmake_minimum_required(VERSION 3.20)

project(SequoiaTestFrameworkHarness DESCRIPTION "End-to-end testing of the Sequoia testing framework"
                                    LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestFrameworkHarness TestFrameworkHarnessMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(TestFrameworkHarness PRIVATE ${TestDir}/TestFramework/TestRunnerEndToEndFreeTest.cpp
                                            ${TestDir}/Parsing/CommandLineArgumentsTestingUtilities.cpp)

sequoia_finalize_self(TestFrameworkHarness ${TestDir} Tests)
