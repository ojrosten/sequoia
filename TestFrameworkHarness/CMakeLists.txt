cmake_minimum_required(VERSION 3.20)

project(SequioaTestFrameworkHarness DESCRIPTION "End-to-end testing of the Sequoia testing framework"
                                    LANGUAGES CXX)

include(../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestFrameworkHarness TestFrameworkHarnessMain.cpp)

target_sources(TestFrameworkHarness PRIVATE ../Tests/TestFramework/TestRunnerEndToEndFreeTest.cpp)

sequoia_finalize_self(TestFrameworkHarness ../Tests/TestFramework)
