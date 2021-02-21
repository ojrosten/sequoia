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

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR}) 

include(${CURRENT_DIR}/../build_system/Globbing.cmake)

HEADER_DIRECTORIES(sequoiaHeaderList ../Source/*.hpp)
target_include_directories(TestFramework PUBLIC ${sequoiaHeaderList})

target_compile_features(TestFramework PUBLIC cxx_std_20)