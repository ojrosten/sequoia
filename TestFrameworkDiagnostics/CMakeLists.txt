cmake_minimum_required(VERSION 3.19)

project(SequioaTestFrameworkDiagnostics DESCRIPTION "The Sequoia Library plus diagnostics for the testing framework"
                                        LANGUAGES CXX)

add_executable(TestFrameworkDiagnostics TestFrameworkDiagnosticsMain.cpp)

set(TestDir "../Tests")

target_sources(TestFrameworkDiagnostics PRIVATE
               ${TestDir}/TestFramework/CoreDiagnostics.cpp
               ${TestDir}/TestFramework/FuzzyTestDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyScopedAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableMoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableRegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/PerformanceTestDiagnostics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/ScopedAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnosticsUtilities.cpp
               ${TestDir}/TestFramework/TestRunnerTest.cpp
               ${TestDir}/TestFramework/TestRunnerTestCreation.cpp
               ${TestDir}/TestFramework/TestRunnerProjectCreation.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTest.cpp
               ${TestDir}/Parsing/CommandLineArgumentsDiagnostics.cpp
               ${TestDir}/Runtime/FactoryTestingDiagnostics.cpp
               ${TestDir}/Runtime/FactoryTest.cpp)

SET(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

file(GLOB_RECURSE HeaderFiles ${TestDir}/TestFramework/*.hpp)
target_sources(TestFrameworkDiagnostics PRIVATE ${HeaderFiles})

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE_SEQUOIA(TestFrameworkDiagnostics)

