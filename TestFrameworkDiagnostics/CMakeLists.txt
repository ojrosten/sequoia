cmake_minimum_required(VERSION 3.15)

project(SequioaTestFrameworkDiagnostics DESCRIPTION "The Sequoia Library plus diagnostics for the testing framework"
                                        LANGUAGES CXX)

add_executable(TestFrameworkDiagnostics TestFrameworkDiagnosticsMain.cpp
                              ../Tests/TestFramework/CoreDiagnostics.cpp
                              ../Tests/TestFramework/FuzzyTestDiagnostics.cpp
                              ../Tests/TestFramework/MoveOnlyAllocationTestDiagnostics.cpp
                              ../Tests/TestFramework/MoveOnlyScopedAllocationTestDiagnostics.cpp
                              ../Tests/TestFramework/MoveOnlyTestDiagnostics.cpp
                              ../Tests/TestFramework/OrderableMoveOnlyTestDiagnostics.cpp
                              ../Tests/TestFramework/OrderableRegularTestDiagnostics.cpp
                              ../Tests/TestFramework/PerformanceTestDiagnostics.cpp
                              ../Tests/TestFramework/RegularAllocationTestDiagnostics.cpp
                              ../Tests/TestFramework/ScopedAllocationTestDiagnostics.cpp
                              ../Tests/TestFramework/TestRunnerDiagnostics.cpp
                              ../Tests/TestFramework/TestRunnerDiagnosticsUtilities.cpp
                              ../Tests/TestFramework/TestRunnerTest.cpp
                              ../Tests/TestFramework/TestRunnerTestCreation.cpp
                              ../Tests/TestFramework/TestRunnerProjectCreation.cpp
                              ../Tests/Parsing/CommandLineArgumentsTest.cpp
                              ../Tests/Parsing/CommandLineArgumentsDiagnostics.cpp
                              ../Tests/Runtime/FactoryTestingDiagnostics.cpp
                              ../Tests/Runtime/FactoryTest.cpp)

SET(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

include(${BUILD_SYS}/CompilerOptions.cmake)
include(${BUILD_SYS}/TestFrameworkLibrary.cmake)
include(${BUILD_SYS}/TestIncludeDirectories.cmake)
include(${BUILD_SYS}/LinkToLibraries.cmake)

ADD_TEST_INCLUDE_DIRS(TestFrameworkDiagnostics)
LINK_LIBRARIES(TestFrameworkDiagnostics)

target_compile_features(TestFrameworkDiagnostics PUBLIC cxx_std_20)