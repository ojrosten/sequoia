cmake_minimum_required(VERSION 3.19)

set(SourceList sequoia/TestFramework/Advice.cpp
    sequoia/Parsing/CommandLineArguments.cpp
    sequoia/PlatformSpecific/Helpers.cpp
    sequoia/Streaming/Streaming.cpp
    sequoia/TestFramework/AllocationCheckersDetails.cpp
    sequoia/TestFramework/FileEditors.cpp
    sequoia/TestFramework/FileSystem.cpp
    sequoia/TestFramework/FreeTestCore.cpp
    sequoia/TestFramework/Output.cpp
    sequoia/TestFramework/PerformanceTestCore.cpp
    sequoia/TestFramework/SemanticsCheckersDetails.cpp
    sequoia/TestFramework/Summary.cpp
    sequoia/TestFramework/TestFamily.cpp
    sequoia/TestFramework/TestRunner.cpp
    sequoia/TextProcessing/Indent.cpp
    sequoia/TextProcessing/Patterns.cpp
    sequoia/TextProcessing/Substitutions.cpp)

add_library(TestFramework STATIC ${SourceList})

target_include_directories(TestFramework PUBLIC .)

file(GLOB_RECURSE HeaderFiles *.hpp)
target_sources(TestFramework PRIVATE ${HeaderFiles})

target_compile_features(TestFramework PUBLIC cxx_std_20)
