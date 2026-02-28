cmake_minimum_required(VERSION 3.20)

include(${CMAKE_CURRENT_LIST_DIR}/../../build_system/Utilities.cmake)

sequoia_init()

set(SourceList
    Maths/Graph/GraphErrors.cpp
    Parsing/CommandLineArguments.cpp
    PlatformSpecific/Helpers.cpp
    Runtime/ShellCommands.cpp
    Streaming/Streaming.cpp
    TestFramework/Advice.cpp
    TestFramework/AllocationCheckersCore.cpp
    TestFramework/AllocationCheckersDetails.cpp
    TestFramework/Commands.cpp
    TestFramework/ConcreteTypeCheckers.cpp
    TestFramework/DependencyAnalyzer.cpp
    TestFramework/FailureInfo.cpp
    TestFramework/FileEditors.cpp
    TestFramework/FileSystemUtilities.cpp
    TestFramework/FreeTestCore.cpp
    TestFramework/IndividualTestPaths.cpp
    TestFramework/MaterialsUpdater.cpp
    TestFramework/MoveOnlyTestCore.cpp
    TestFramework/Output.cpp
    TestFramework/PerformanceTestCore.cpp
    TestFramework/ProjectCreator.cpp
    TestFramework/ProjectPaths.cpp
    TestFramework/RegularTestCore.cpp
    TestFramework/SemanticsCheckersDetails.cpp
    TestFramework/Summary.cpp
    TestFramework/TestCreator.cpp
    TestFramework/TestLogger.cpp
    TestFramework/TestRunner.cpp
    TestFramework/TestRunnerUtilities.cpp
    TextProcessing/Indent.cpp
    TextProcessing/Patterns.cpp
    TextProcessing/Substitutions.cpp)

add_library(sequoia STATIC ${SourceList})

target_include_directories(sequoia PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(sequoia)
