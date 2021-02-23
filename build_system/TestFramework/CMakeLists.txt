cmake_minimum_required(VERSION 3.19)

SET(SOURCE_DIR ../../Source)

add_library(TestFramework STATIC ${SOURCE_DIR}/TestFramework/Advice.cpp
                                 ${SOURCE_DIR}/TestFramework/AllocationCheckersDetails.cpp
                                 ${SOURCE_DIR}/TestFramework/FileEditors.cpp
                                 ${SOURCE_DIR}/TestFramework/FileSystem.cpp
								 ${SOURCE_DIR}/TestFramework/FreeTestCore.cpp
                                 ${SOURCE_DIR}/TestFramework/Output.cpp
								 ${SOURCE_DIR}/TestFramework/PerformanceTestCore.cpp
								 ${SOURCE_DIR}/TestFramework/SemanticsCheckersDetails.cpp
                                 ${SOURCE_DIR}/TestFramework/Summary.cpp
                                 ${SOURCE_DIR}/TestFramework/TestFamily.cpp
                                 ${SOURCE_DIR}/TestFramework/TestRunner.cpp
                                 ${SOURCE_DIR}/Formatting/Indent.cpp
                                 ${SOURCE_DIR}/Parsing/CommandLineArguments.cpp
                                 ${SOURCE_DIR}/PlatformSpecific/Preprocessor.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR}) 

include(${CMAKE_CURRENT_SOURCE_DIR}/../Globbing.cmake)

HEADER_DIRECTORIES(sequoiaHeaderList ${SOURCE_DIR}/*.hpp)
target_include_directories(TestFramework PUBLIC ${sequoiaHeaderList})

target_compile_features(TestFramework PUBLIC cxx_std_20)
