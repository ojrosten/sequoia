cmake_minimum_required(VERSION 3.15)

project(SequioaMinimal DESCRIPTION "The Sequoia Library plus executable"
                       LANGUAGES CXX)

if (MSVC)
    add_compile_options(/W4)
    add_definitions(/bigobj)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

add_executable(SequoiaMinimal TestMinimalMain.cpp)
target_include_directories(SequoiaMinimal PUBLIC ../TestCommon)
target_include_directories(SequoiaMinimal PUBLIC ../Tests)

add_library(TestFramework STATIC ../Source/TestFramework/Advice.cpp
                                 ../Source/TestFramework/AllocationCheckersDetails.cpp
                                 ../Source/TestFramework/FileEditors.cpp
                                 ../Source/TestFramework/FileSystem.cpp
                                 ../Source/TestFramework/Output.cpp
                                 ../Source/TestFramework/Summary.cpp
                                 ../Source/TestFramework/TestFamily.cpp
                                 ../Source/TestFramework/TestRunner.cpp
                                 ../Source/Formatting/Indent.cpp
                                 ../Source/Parsing/CommandLineArguments.cpp)

include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/Globbing.cmake)

HEADER_DIRECTORIES(sequoiaHeaderList ../Source/*.hpp)
target_include_directories(TestFramework PUBLIC ${sequoiaHeaderList})


HEADER_DIRECTORIES(testHeaderList ../Tests/*.hpp)
target_include_directories(SequoiaMinimal PUBLIC ${testHeaderList})

target_link_libraries(SequoiaMinimal PUBLIC TestFramework)


target_compile_features(SequoiaMinimal PUBLIC cxx_std_20)
target_compile_features(TestFramework PUBLIC cxx_std_20)
