cmake_minimum_required(VERSION 3.20)

project(SequoiaTestFrameworkDiagnostics DESCRIPTION "The Sequoia Library plus diagnostics for the testing framework"
                                        LANGUAGES CXX)

include(../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestFrameworkDiagnostics TestFrameworkDiagnosticsMain.cpp)

set(TestDir ../Tests)

target_sources(TestFrameworkDiagnostics PRIVATE
               ${TestDir}/Parsing/CommandLineArgumentsDiagnostics.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTest.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTestingUtilities.cpp
               ${TestDir}/Runtime/FactoryTest.cpp
               ${TestDir}/Runtime/FactoryTestingDiagnostics.cpp
               ${TestDir}/Runtime/ShellCommandsTest.cpp
               ${TestDir}/Runtime/ShellCommandsTestingDiagnostics.cpp
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
               ${TestDir}/TestFramework/FunctionFreeDiagnostics.cpp
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
               ${TestDir}/TestFramework/PathFreeDiagnostics.cpp
               ${TestDir}/TestFramework/PerformanceTestDiagnostics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestFalseNegativeDiagnostics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestFalsePositiveDiagnostics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestFalsePositiveDiagnosticsBrokenSemantics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestFalsePositiveDiagnosticsBrokenValueSemantics.cpp
               ${TestDir}/TestFramework/RegularAllocationTestFalsePositiveDiagnosticsInefficientOperations.cpp
               ${TestDir}/TestFramework/RegularStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/RegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/RelationalTestDiagnostics.cpp
               ${TestDir}/TestFramework/ScopedAllocationTestFalseNegativeDiagnostics.cpp
               ${TestDir}/TestFramework/ScopedAllocationTestFalseNegativeDiagnosticsMixed.cpp
               ${TestDir}/TestFramework/ScopedAllocationTestFalsePositiveDiagnostics.cpp
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

sequoia_finalize_self(TestFrameworkDiagnostics ${TestDir}/TestFramework)

