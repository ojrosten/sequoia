cmake_minimum_required(VERSION 3.19)

set(SourceDir ../../Source)

set(SourceList ${SourceDir}/TestFramework/Advice.cpp
               ${SourceDir}/TestFramework/AllocationCheckersDetails.cpp
               ${SourceDir}/TestFramework/FileEditors.cpp
               ${SourceDir}/TestFramework/FileSystem.cpp
               ${SourceDir}/TestFramework/FreeTestCore.cpp
               ${SourceDir}/TestFramework/Output.cpp
               ${SourceDir}/TestFramework/PerformanceTestCore.cpp
               ${SourceDir}/TestFramework/SemanticsCheckersDetails.cpp
               ${SourceDir}/TestFramework/Summary.cpp
               ${SourceDir}/TestFramework/TestFamily.cpp
               ${SourceDir}/TestFramework/TestRunner.cpp
               ${SourceDir}/Formatting/Indent.cpp
               ${SourceDir}/Parsing/CommandLineArguments.cpp
               ${SourceDir}/PlatformSpecific/Helpers.cpp)

add_library(TestFramework STATIC ${SourceList})

include(${CMAKE_CURRENT_SOURCE_DIR}/../Globbing.cmake)

HEADER_DIRECTORIES(SequoiaDirs ${SourceDir}/*.hpp)
target_include_directories(TestFramework PUBLIC ${SequoiaDirs})

file(GLOB_RECURSE HeaderFiles ${SourceDir}/*.hpp)
target_sources(TestFramework PRIVATE ${HeaderFiles})

target_compile_features(TestFramework PUBLIC cxx_std_20)
