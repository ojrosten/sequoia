cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestSandbox.cpp)

target_sources(TestMain PRIVATE
               ../Tests/Maybe/MaybeTestingDiagnostics.cpp
               ../Tests/Maybe/MaybeTest.cpp
               ../Tests/Utilities/IteratorTestingDiagnostics.cpp
               ../Tests/Utilities/IteratorTest.cpp
               ../Tests/Stuff/FooTestingDiagnostics.cpp
               ../Tests/Stuff/FooTest.cpp
               ../Tests/Partners/CoupleTestingDiagnostics.cpp
               ../Tests/Partners/CoupleTest.cpp
               ../Tests/Utilities/UtilitiesFreeTest.cpp
               ../Tests/Stuff/BazzerFreeTest.cpp
               ../Tests/Stuff/FooAllocationTest.cpp
               ../Tests/Stuff/ContainerTestingDiagnostics.cpp
               ../Tests/Stuff/ContainerTest.cpp
               ../Tests/Stuff/ContainerAllocationTest.cpp
               ../Tests/Stuff/ContainerPerformanceTest.cpp
               ../Tests/Utilities/VariadicTestingDiagnostics.cpp
               ../Tests/Utilities/VariadicTest.cpp)

SET(BUILD_SYS SEQUOIA_ROOT/build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE(TestMain)

