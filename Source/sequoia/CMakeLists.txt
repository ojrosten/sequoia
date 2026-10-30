cmake_minimum_required(VERSION 4.2)

include(${CMAKE_CURRENT_LIST_DIR}/../../build_system/Utilities.cmake)

sequoia_init()

set(SourceList
    Maths/Graph/GraphErrors.cpp
    Parsing/CommandLineArguments.cpp
    Parsing/CommandLineMessages.cpp
    PlatformSpecific/Helpers.cpp
    Runtime/ShellCommands.cpp
    Streaming/Streaming.cpp
    TestFramework/AllocationCheckersCore.cpp
    TestFramework/AllocationCheckersDetails.cpp
    TestFramework/BuildArtefacts.cpp
    TestFramework/CMakeCache.cpp
    TestFramework/CoarseSleeps.cpp
    TestFramework/Commands.cpp
    TestFramework/ConcreteTypeCheckers.cpp
    TestFramework/DependencyAnalyzer.cpp
    TestFramework/DumpComparison.cpp
    TestFramework/FailureInfo.cpp
    TestFramework/FailureReporting.cpp
    TestFramework/FileEditors.cpp
    TestFramework/FileSystemUtilities.cpp
    TestFramework/FreeTestCore.cpp
    TestFramework/IndividualTestPaths.cpp
    TestFramework/InstabilityAnalysis.cpp
    TestFramework/InstabilityProjection.cpp
    TestFramework/MaterialsUpdater.cpp
    TestFramework/MoveOnlyTestCore.cpp
    TestFramework/Output.cpp
    TestFramework/PerformanceSummary.cpp
    TestFramework/PerformanceTestCore.cpp
    TestFramework/ProjectCreator.cpp
    TestFramework/ProjectPaths.cpp
    TestFramework/RegularTestCore.cpp
    TestFramework/ReturnCodes.cpp
    TestFramework/SemanticsCheckersDetails.cpp
    TestFramework/Summary.cpp
    TestFramework/TestCreator.cpp
    TestFramework/TestExecutionRecord.cpp
    TestFramework/TestLogger.cpp
    TestFramework/TestRunner.cpp
    TestFramework/TestRunnerUtilities.cpp
    TestFramework/TimingUtilities.cpp
    TestFramework/VersionedOutput.cpp
    TextProcessing/Indent.cpp
    TextProcessing/Patterns.cpp
    TextProcessing/Substitutions.cpp)

add_library(sequoia STATIC ${SourceList})

target_compile_definitions(sequoia PRIVATE SEQUOIA_BUILD_CONFIGURATION="$<CONFIG>")

target_include_directories(sequoia PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)

sequoia_finalize_library(sequoia)
