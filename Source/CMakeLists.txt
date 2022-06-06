cmake_minimum_required(VERSION 3.20)

include(../build_system/Utilities.cmake)

sequoia_init()

set(SourceList sequoia/TestFramework/Advice.cpp
    sequoia/Parsing/CommandLineArguments.cpp
    sequoia/PlatformSpecific/Helpers.cpp
    sequoia/Runtime/ShellCommands.cpp
    sequoia/Streaming/Streaming.cpp
    sequoia/TestFramework/AllocationCheckersCore.cpp
    sequoia/TestFramework/AllocationCheckersDetails.cpp
    sequoia/TestFramework/ConcreteTypeCheckers.cpp
    sequoia/TestFramework/DependencyAnalyzer.cpp
    sequoia/TestFramework/FailureInfo.cpp
    sequoia/TestFramework/FileEditors.cpp
    sequoia/TestFramework/FileSystemUtilities.cpp
    sequoia/TestFramework/FreeTestCore.cpp
    sequoia/TestFramework/MaterialsUpdater.cpp
    sequoia/TestFramework/MoveOnlyTestCore.cpp
    sequoia/TestFramework/Output.cpp
    sequoia/TestFramework/PerformanceTestCore.cpp
    sequoia/TestFramework/ProjectCreator.cpp
    sequoia/TestFramework/ProjectPaths.cpp
    sequoia/TestFramework/RegularTestCore.cpp
    sequoia/TestFramework/SemanticsCheckersDetails.cpp
    sequoia/TestFramework/Summary.cpp
    sequoia/TestFramework/TestCreator.cpp
    sequoia/TestFramework/TestFamily.cpp
    sequoia/TestFramework/TestFamilySelector.cpp
    sequoia/TestFramework/TestLogger.cpp
    sequoia/TestFramework/TestRunner.cpp
    sequoia/TestFramework/TestRunnerUtilities.cpp
    sequoia/TextProcessing/Indent.cpp
    sequoia/TextProcessing/Patterns.cpp
    sequoia/TextProcessing/Substitutions.cpp)

add_library(TestFramework STATIC ${SourceList})

target_include_directories(TestFramework PUBLIC .)

sequoia_finalize_library(TestFramework)
