cmake_minimum_required(VERSION 3.19)

project(GeneratedProjectTests LANGUAGES CXX)

add_executable(TestAll TestAllMain.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem ../../../../../../build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

add_subdirectory(${SourceDir} GeneratedProject)
target_link_libraries(TestAll PUBLIC GeneratedProject)

target_sources(TestAll PRIVATE
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
               ${TestDir}/Unstable/FlipperFreeTest.cpp
               ${TestDir}/Utilities/ContainerAllocationTest.cpp
               ${TestDir}/Utilities/ContainerPerformanceTest.cpp
               ${TestDir}/Utilities/Thing/UniqueThingTest.cpp
               ${TestDir}/Utilities/Thing/UniqueThingTestingDiagnostics.cpp
               ${TestDir}/Utilities/UsefulThingsFreeTest.cpp
               ${TestDir}/Utilities/UtilitiesFreeTest.cpp)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestAll PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TestAll)
endif()

FINALIZE(TestAll)

