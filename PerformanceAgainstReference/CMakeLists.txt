cmake_minimum_required(VERSION 4.2)

project(SequoiaPerformanceAgainstReference DESCRIPTION "The Sequoia Library's performance against a reference tree's"
                                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

# The executable's test compares this tree's runner with the runner of the
# tree whose root this variable names. A path naming this tree would compare the
# tree with itself, and the check would pass whatever the runner did.
set(SEQUOIA_PERFORMANCE_REFERENCE_ROOT "" CACHE PATH
    "The root of the sequoia tree against which to compare performance")
file(REAL_PATH "${SEQUOIA_PERFORMANCE_REFERENCE_ROOT}" referenceRoot)
file(REAL_PATH "${CMAKE_CURRENT_LIST_DIR}/.." thisRoot)
if(NOT EXISTS "${referenceRoot}/Source/sequoia/CMakeLists.txt")
    message(FATAL_ERROR "SEQUOIA_PERFORMANCE_REFERENCE_ROOT must name the root of a sequoia tree; "
                        "it is '${SEQUOIA_PERFORMANCE_REFERENCE_ROOT}'")
elseif(referenceRoot STREQUAL thisRoot)
    message(FATAL_ERROR "SEQUOIA_PERFORMANCE_REFERENCE_ROOT names this tree, '${thisRoot}', not a reference")
endif()

add_executable(PerformanceAgainstReference PerformanceAgainstReferenceMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

target_sources(PerformanceAgainstReference PRIVATE ${TestDir}/TestFramework/RunnerPerformanceAgainstReferenceTest.cpp)

sequoia_finalize_self(PerformanceAgainstReference ${TestDir} Tests)
