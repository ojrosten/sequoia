cmake_minimum_required(VERSION 3.20)

project(SequoiaTestFrameworkDiagnostics DESCRIPTION "The Sequoia Library plus diagnostics for the testing framework"
                                        LANGUAGES CXX)

find_package(Threads REQUIRED)

add_executable(TestFrameworkDiagnostics TestFrameworkDiagnosticsMain.cpp)

set(TestDir "../Tests")

target_sources(TestFrameworkDiagnostics PRIVATE
               ${TestDir}/Parsing/CommandLineArgumentsDiagnostics.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTest.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTestingUtilities.cpp
               ${TestDir}/Runtime/FactoryTest.cpp
               ${TestDir}/Runtime/FactoryTestingDiagnostics.cpp
               ${TestDir}/TestFramework/CoreDiagnostics.cpp
               ${TestDir}/TestFramework/DependencyAnalyzerFreeTest.cpp
               ${TestDir}/TestFramework/FailureInfoTest.cpp
               ${TestDir}/TestFramework/FailureInfoTestingDiagnostics.cpp
               ${TestDir}/TestFramework/FileSystemFreeTest.cpp
               ${TestDir}/TestFramework/MaterialsUpdaterFreeTest.cpp
               ${TestDir}/TestFramework/MoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyScopedAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableMoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableMoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableRegularAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableRegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/OutputFreeTest.cpp
               ${TestDir}/TestFramework/PerformanceTestDiagnostics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/RegularStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/RegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/RelationalTestDiagnostics.cpp
               ${TestDir}/TestFramework/ScopedAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnosticsUtilities.cpp
               ${TestDir}/TestFramework/TestRunnerProjectCreation.cpp
               ${TestDir}/TestFramework/TestRunnerTest.cpp
               ${TestDir}/TestFramework/TestRunnerTestCreation.cpp
               ${TestDir}/TextProcessing/IndentFreeTest.cpp
               ${TestDir}/TextProcessing/PatternsFreeTest.cpp
               ${TestDir}/TextProcessing/SubstitutionsFreeTest.cpp)

set(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

file(GLOB_RECURSE HeaderFiles ${TestDir}/TestFramework/*.hpp)
target_sources(TestFrameworkDiagnostics PRIVATE ${HeaderFiles})

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE_SEQUOIA(TestFrameworkDiagnostics)

