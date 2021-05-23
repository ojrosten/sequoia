cmake_minimum_required(VERSION 3.19)

project(MyProjectTests LANGUAGES CXX)

add_executable(TestMain TestSandbox.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem SEQUOIA_ROOT/build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

add_subdirectory(${CURRENT_DIR}/../Source MyProject)

target_sources(TestMain PRIVATE
               ${TestDir}/Maybe/MaybeTestingDiagnostics.cpp
               ${TestDir}/Maybe/MaybeTest.cpp
               ${TestDir}/Utilities/IteratorTestingDiagnostics.cpp
               ${TestDir}/Utilities/IteratorTest.cpp
               ${TestDir}/Stuff/WidgetTestingDiagnostics.cpp
               ${TestDir}/Stuff/WidgetTest.cpp
               ${TestDir}/Thingummies/ThingummyTestingDiagnostics.cpp
               ${TestDir}/Thingummies/ThingummyTest.cpp
               ${TestDir}/Stuff/ContainerTestingDiagnostics.cpp
               ${TestDir}/Stuff/ContainerTest.cpp
               ${TestDir}/CoupleTestingDiagnostics.cpp
               ${TestDir}/CoupleTest.cpp
               ${TestDir}/Stuff/ThingsTestingDiagnostics.cpp
               ${TestDir}/Stuff/ThingsTest.cpp
               ${TestDir}/Stuff/FooTestingDiagnostics.cpp
               ${TestDir}/Stuff/FooTest.cpp
               ${TestDir}/Utilities/VariadicTestingDiagnostics.cpp
               ${TestDir}/Utilities/VariadicTest.cpp
               ${TestDir}/Utilities/MultipleTestingDiagnostics.cpp
               ${TestDir}/Utilities/MultipleTest.cpp
               ${TestDir}/Utilities/UtilitiesFreeTest.cpp
               ${TestDir}/Stuff/BazzerFreeTest.cpp
               ${TestDir}/Stuff/DoohickyFreeTest.cpp
               ${TestDir}/Global/Stuff/GlobalFreeTest.cpp
               ${TestDir}/Global/Stuff/DefsFreeTest.cpp
               ${TestDir}/Stuff/ContainerAllocationTest.cpp
               ${TestDir}/Stuff/FooAllocationTest.cpp
               ${TestDir}/Stuff/ContainerPerformanceTest.cpp)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestMain PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestMain PRIVATE ${SourceDir})
target_include_directories(TestMain PRIVATE ${TestDir})

FINALIZE(TestMain)

