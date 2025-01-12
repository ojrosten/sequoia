cmake_minimum_required(VERSION 3.20)

project(SequoiaTestFrameworkDiagnostics DESCRIPTION "The Sequoia Library plus diagnostics for the testing framework"
                                        LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestFrameworkDiagnostics TestFrameworkDiagnosticsMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(TestFrameworkDiagnostics PRIVATE
               ${TestDir}/Parsing/CommandLineArgumentsDiagnostics.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTest.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTestingUtilities.cpp
               ${TestDir}/Runtime/FactoryTest.cpp
               ${TestDir}/Runtime/FactoryTestingDiagnostics.cpp
               ${TestDir}/Runtime/ShellCommandsTest.cpp
               ${TestDir}/Runtime/ShellCommandsTestingDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/MoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/MoveOnlyScopedAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/OrderableMoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/OrderableRegularAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnosticsBrokenSemantics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnosticsBrokenValueSemantics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnosticsInefficientOperations.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnosticsMixed.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnosticsThreeLevel.cpp
               ${TestDir}/TestFramework/BasicTestInterfaceFreeTest.cpp
               ${TestDir}/TestFramework/ChronoFreeDiagnostics.cpp
               ${TestDir}/TestFramework/CommandsFreeTest.cpp
               ${TestDir}/TestFramework/ComplexFreeDiagnostics.cpp
               ${TestDir}/TestFramework/ContainerFreeDiagnostics.cpp
               ${TestDir}/TestFramework/DependencyAnalyzerFreeTest.cpp
               ${TestDir}/TestFramework/ElementaryFreeDiagnostics.cpp
               ${TestDir}/TestFramework/ExceptionsFreeDiagnostics.cpp
               ${TestDir}/TestFramework/FailureInfoTest.cpp
               ${TestDir}/TestFramework/FailureInfoTestingDiagnostics.cpp
               ${TestDir}/TestFramework/FileSystemUtilitiesFreeTest.cpp
               ${TestDir}/TestFramework/FreeCheckersMetaFreeTest.cpp
               ${TestDir}/TestFramework/FunctionFreeDiagnostics.cpp
               ${TestDir}/TestFramework/IndividualTestPathsFreeTest.cpp
               ${TestDir}/TestFramework/MaterialsUpdaterFreeTest.cpp
               ${TestDir}/TestFramework/MoveOnlyStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableMoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableRegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/OutputFreeTest.cpp
               ${TestDir}/TestFramework/PathFreeDiagnostics.cpp
               ${TestDir}/TestFramework/PerformanceTestDiagnostics.cpp
               ${TestDir}/TestFramework/RegularStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/RegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/RelationalTestDiagnostics.cpp
               ${TestDir}/TestFramework/SmartPointerFreeDiagnostics.cpp
               ${TestDir}/TestFramework/StringFreeDiagnostics.cpp
               ${TestDir}/TestFramework/SumTypesFreeDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnosticsUtilities.cpp
               ${TestDir}/TestFramework/TestRunnerPerformanceTest.cpp
               ${TestDir}/TestFramework/TestRunnerProjectCreation.cpp
               ${TestDir}/TestFramework/TestRunnerTest.cpp
               ${TestDir}/TestFramework/TestRunnerTestCreation.cpp
               ${TestDir}/TextProcessing/IndentFreeTest.cpp
               ${TestDir}/TextProcessing/PatternsFreeTest.cpp
               ${TestDir}/TextProcessing/SubstitutionsFreeTest.cpp)

sequoia_finalize_self(TestFrameworkDiagnostics ${TestDir} Tests)

