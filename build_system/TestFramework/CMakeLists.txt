cmake_minimum_required(VERSION 3.19)

set(SourceDir ../../Source)
set(SeqDir ${SourceDir}/sequoia)

set(SourceList ${SeqDir}/TestFramework/Advice.cpp
               ${SeqDir}/TestFramework/AllocationCheckersDetails.cpp
               ${SeqDir}/TestFramework/FileEditors.cpp
               ${SeqDir}/TestFramework/FileSystem.cpp
               ${SeqDir}/TestFramework/FreeTestCore.cpp
               ${SeqDir}/TestFramework/Output.cpp
               ${SeqDir}/TestFramework/PerformanceTestCore.cpp
               ${SeqDir}/TestFramework/SemanticsCheckersDetails.cpp
               ${SeqDir}/TestFramework/Summary.cpp
               ${SeqDir}/TestFramework/TestFamily.cpp
               ${SeqDir}/TestFramework/TestRunner.cpp
               ${SeqDir}/Formatting/Indent.cpp
               ${SeqDir}/Parsing/CommandLineArguments.cpp
               ${SeqDir}/PlatformSpecific/Helpers.cpp)

add_library(TestFramework STATIC ${SourceList})

target_include_directories(TestFramework PUBLIC ${SourceDir})

file(GLOB_RECURSE HeaderFiles ${SeqDir}/*.hpp)
target_sources(TestFramework PRIVATE ${HeaderFiles})

target_compile_features(TestFramework PUBLIC cxx_std_20)
