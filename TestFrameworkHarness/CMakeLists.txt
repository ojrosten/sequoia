# 3.28: consumers of a module library must themselves be scanned for imports,
# and under CMP0155's OLD behaviour they are not - every import then fails with
# "module not found" even though the library built.
cmake_minimum_required(VERSION 3.28)

project(SequoiaTestFrameworkHarness DESCRIPTION "End-to-end testing of the Sequoia testing framework"
                                    LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestFrameworkHarness TestFrameworkHarnessMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(TestFrameworkHarness PRIVATE ${TestDir}/TestFramework/TestRunnerEndToEndFreeTest.cpp
                                            ${TestDir}/TestFramework/TestRunnerProjectFiles.cpp
                                            ${TestDir}/Parsing/CommandLineArgumentsTestingUtilities.cpp)

sequoia_finalize_self(TestFrameworkHarness ${TestDir} Tests)
