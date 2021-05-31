cmake_minimum_required(VERSION 3.19)

project(GeneratedProjectTests LANGUAGES CXX)

add_executable(TestMain TestMain.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem ../../../../../../build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

add_subdirectory(${SourceDir} GeneratedProject)
target_link_libraries(TestMain PUBLIC GeneratedProject)

target_sources(TestMain PRIVATE
               ${TestDir}/HouseAllocationTest.cpp
               ${TestDir}/Maths/ProbabilityTest.cpp
               ${TestDir}/Maths/ProbabilityTestingDiagnostics.cpp
               ${TestDir}/Maybe/MaybeTest.cpp
               ${TestDir}/Maybe/MaybeTestingDiagnostics.cpp
               ${TestDir}/Stuff/BarFreeTest.cpp
               ${TestDir}/Stuff/FooTest.cpp
               ${TestDir}/Stuff/FooTestingDiagnostics.cpp
               ${TestDir}/Stuff/OldschoolTest.cpp
               ${TestDir}/Stuff/OldschoolTestingDiagnostics.cpp
               ${TestDir}/Utilities/ContainerAllocationTest.cpp
               ${TestDir}/Utilities/ContainerPerformanceTest.cpp
               ${TestDir}/Utilities/UniqueThingTest.cpp
               ${TestDir}/Utilities/UniqueThingTestingDiagnostics.cpp
               ${TestDir}/Utilities/UsefulThingsFreeTest.cpp
               ${TestDir}/Utilities/UtilitiesFreeTest.cpp)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestMain PRIVATE ${SourceDir})
target_include_directories(TestMain PRIVATE ${TestDir})

FINALIZE(TestMain)

