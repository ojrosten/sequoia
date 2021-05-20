cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestSandbox.cpp)

target_sources(TestMain PRIVATE
               ${TestDir}/Maybe/MaybeTestingDiagnostics.cpp
               ${TestDir}/Maybe/MaybeTest.cpp
               ${TestDir}/Utilities/IteratorTestingDiagnostics.cpp
               ${TestDir}/Utilities/IteratorTest.cpp
               ${TestDir}/Stuff/WidgetTestingDiagnostics.cpp
               ${TestDir}/Stuff/WidgetTest.cpp
               ${TestDir}/Thingummies/ThingummyTestingDiagnostics.cpp
               ${TestDir}/Thingummies/ThingummyTest.cpp
               ${TestDir}/Stuff/FooTestingDiagnostics.cpp
               ${TestDir}/Stuff/FooTest.cpp
               ${TestDir}/CoupleTestingDiagnostics.cpp
               ${TestDir}/CoupleTest.cpp
               ${TestDir}/Stuff/ThingsTestingDiagnostics.cpp
               ${TestDir}/Stuff/ThingsTest.cpp
               ${TestDir}/Utilities/UtilitiesFreeTest.cpp
               ${TestDir}/Stuff/BazzerFreeTest.cpp
               ${TestDir}/Stuff/DoohickyFreeTest.cpp
               ${TestDir}/Stuff/FooAllocationTest.cpp
               ${TestDir}/Stuff/ContainerTestingDiagnostics.cpp
               ${TestDir}/Stuff/ContainerTest.cpp
               ${TestDir}/Stuff/ContainerAllocationTest.cpp
               ${TestDir}/Stuff/ContainerPerformanceTest.cpp
               ${TestDir}/Utilities/VariadicTestingDiagnostics.cpp
               ${TestDir}/Utilities/VariadicTest.cpp)

SET(BUILD_SYS SEQUOIA_ROOT/build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE(TestMain)

