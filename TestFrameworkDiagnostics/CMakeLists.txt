cmake_minimum_required(VERSION 3.15)

project(SequioaTestFrameworkDiagnostics DESCRIPTION "The Sequoia Library plus diagnostics for the testing framework"
                       LANGUAGES CXX)

if (MSVC)
    add_compile_options(/W4)
    add_definitions(/bigobj)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

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

target_include_directories(TestFrameworkDiagnostics PUBLIC ../TestCommon)
target_include_directories(TestFrameworkDiagnostics PUBLIC ../Tests)

add_library(TestFramework STATIC ../Source/TestFramework/Advice.cpp
                                 ../Source/TestFramework/AllocationCheckersDetails.cpp
                                 ../Source/TestFramework/FileEditors.cpp
                                 ../Source/TestFramework/FileSystem.cpp
                                 ../Source/TestFramework/FreeTestCore.cpp
                                 ../Source/TestFramework/Output.cpp
                                 ../Source/TestFramework/PerformanceTestCore.cpp
                                 ../Source/TestFramework/SemanticsCheckersDetails.cpp
                                 ../Source/TestFramework/Summary.cpp
                                 ../Source/TestFramework/TestFamily.cpp
                                 ../Source/TestFramework/TestRunner.cpp
                                 ../Source/Formatting/Indent.cpp
                                 ../Source/Parsing/CommandLineArguments.cpp
                                 ../Source/PlatformSpecific/Preprocessor.cpp)

include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/Globbing.cmake)

HEADER_DIRECTORIES(sequoiaHeaderList ../Source/*.hpp)
target_include_directories(TestFramework PUBLIC ${sequoiaHeaderList})


HEADER_DIRECTORIES(testHeaderList ../Tests/*.hpp)
target_include_directories(TestFrameworkDiagnostics PUBLIC ${testHeaderList})

target_link_libraries(TestFrameworkDiagnostics PUBLIC TestFramework)

if(MSVC)
    target_link_libraries(TestFrameworkDiagnostics PUBLIC winmm)
endif()


target_compile_features(TestFrameworkDiagnostics PUBLIC cxx_std_20)
target_compile_features(TestFramework PUBLIC cxx_std_20)
