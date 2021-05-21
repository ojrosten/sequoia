cmake_minimum_required(VERSION 3.19)

project(GeneratedProjectTests LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem C:/Users/olive/sequoia/output/TestsTemporaryData/TestFramework/TestRunnerProjectCreation/WorkingCopy/FakeProject/build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

#add_subdirectory(${CURRENT_DIR}/../Source GeneratedProject)

target_sources(TestMain PRIVATE)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestMain PRIVATE ${SourceDir})
target_include_directories(TestMain PRIVATE ${TestDir})

FINALIZE(TestMain)

