cmake_minimum_required(VERSION 3.19)

project(MyProjectTests LANGUAGES CXX)

add_executable(TestAll TestSandbox.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem ../build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

add_subdirectory(${SourceDir} MyProject)
target_link_libraries(TestAll PUBLIC MyProject)

target_sources(TestAll PRIVATE
               ${TestDir}/CoupleTest.cpp
               ${TestDir}/CoupleTestingDiagnostics.cpp
               ${TestDir}/Global/Stuff/DefsFreeTest.cpp
               ${TestDir}/Global/Stuff/GlobalFreeTest.cpp
               ${TestDir}/Maths/AngleTest.cpp
               ${TestDir}/Maths/AngleTestingDiagnostics.cpp
               ${TestDir}/Maths/ProbabilityTest.cpp
               ${TestDir}/Maths/ProbabilityTestingDiagnostics.cpp
               ${TestDir}/Maybe/MaybeTest.cpp
               ${TestDir}/Maybe/MaybeTestingDiagnostics.cpp
               ${TestDir}/Stuff/BazagainTest.cpp
               ${TestDir}/Stuff/BazzerTest.cpp
               ${TestDir}/Stuff/ContainerAllocationTest.cpp
               ${TestDir}/Stuff/ContainerPerformanceTest.cpp
               ${TestDir}/Stuff/ContainerTest.cpp
               ${TestDir}/Stuff/ContainerTestingDiagnostics.cpp
               ${TestDir}/Stuff/DoohickyFreeTest.cpp
               ${TestDir}/Stuff/FooAllocationTest.cpp
               ${TestDir}/Stuff/FooTest.cpp
               ${TestDir}/Stuff/FooTestingDiagnostics.cpp
               ${TestDir}/Stuff/ThingsTest.cpp
               ${TestDir}/Stuff/ThingsTestingDiagnostics.cpp
               ${TestDir}/Stuff/WidgetTest.cpp
               ${TestDir}/Stuff/WidgetTestingDiagnostics.cpp
               ${TestDir}/Thingummies/ThingummyTest.cpp
               ${TestDir}/Thingummies/ThingummyTestingDiagnostics.cpp
               ${TestDir}/Utilities/IteratorTest.cpp
               ${TestDir}/Utilities/IteratorTestingDiagnostics.cpp
               ${TestDir}/Utilities/MultipleTest.cpp
               ${TestDir}/Utilities/MultipleTestingDiagnostics.cpp
               ${TestDir}/Utilities/UtilitiesFreeTest.cpp
               ${TestDir}/Utilities/VariadicTest.cpp
               ${TestDir}/Utilities/VariadicTestingDiagnostics.cpp)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestAll PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TestAll)
endif()

FINALIZE(TestAll)

