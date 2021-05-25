cmake_minimum_required(VERSION 3.19)

project(MyProjectTests LANGUAGES CXX)

add_executable(TestMain TestSandbox.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem SEQUOIA_ROOT/build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

add_subdirectory(${SourceDir} MyProject)
target_link_libraries(TestMain PUBLIC MyProject)

target_sources(TestMain PRIVATE
               ${TestDir}/CoupleTest.cpp
               ${TestDir}/CoupleTestingDiagnostics.cpp
               ${TestDir}/Global/Stuff/DefsFreeTest.cpp
               ${TestDir}/Global/Stuff/GlobalFreeTest.cpp
               ${TestDir}/Maybe/MaybeTest.cpp
               ${TestDir}/Maybe/MaybeTestingDiagnostics.cpp
               ${TestDir}/Stuff/BazzerFreeTest.cpp
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
target_sources(TestMain PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestMain PRIVATE ${SourceDir})
target_include_directories(TestMain PRIVATE ${TestDir})

FINALIZE(TestMain)

