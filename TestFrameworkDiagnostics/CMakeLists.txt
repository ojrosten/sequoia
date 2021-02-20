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

include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/CompilerOptions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/TestFrameworkLibrary.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/TestIncludeDirectories.cmake)

ADD_TEST_INCLUDE_DIRS(TestFrameworkDiagnostics)

target_link_libraries(TestFrameworkDiagnostics PUBLIC TestFramework)

if(MSVC)
    target_link_libraries(TestFrameworkDiagnostics PUBLIC winmm)
endif()

target_compile_features(TestFrameworkDiagnostics PUBLIC cxx_std_20)
