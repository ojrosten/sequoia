cmake_minimum_required(VERSION 3.20)

project(MyProjectTests LANGUAGES CXX)

set(Dependencies ../dependencies)
set(BuildSystem ${Dependencies}/sequoia/build_system)
set(TestDir ../Tests)
set(SourceDir ../Source)

include(${BuildSystem}/Utilities.cmake)

sequoia_init()

add_executable(TestAll TestSandbox.cpp)

add_subdirectory(${SourceDir}/myProject MyProject)
target_link_libraries(TestAll PUBLIC MyProject)

target_sources(TestAll PRIVATE
               ${TestDir}/CoupleTest.cpp
               ${TestDir}/CoupleTestingDiagnostics.cpp
               ${TestDir}/Global/Stuff/DefsFreeTest.cpp
               ${TestDir}/Global/Stuff/GlobalFreeTest.cpp
               ${TestDir}/Maths/AngleFreeDiagnostics.cpp
               ${TestDir}/Maths/AngleTest.cpp
               ${TestDir}/Maths/AngleTestingDiagnostics.cpp
               ${TestDir}/Maths/ProbabilityTest.cpp
               ${TestDir}/Maths/ProbabilityTestingDiagnostics.cpp
               ${TestDir}/Maybe/MaybeTest.cpp
               ${TestDir}/Maybe/MaybeTestingDiagnostics.cpp
               ${TestDir}/Stuff/BazagainFreeTest.cpp
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
               ${TestDir}/Utilities/VariadicTestingDiagnostics.cpp
               ${TestDir}/Weather/CloudTest.cpp
               ${TestDir}/Weather/CloudTestingDiagnostics.cpp
               ${TestDir}/hominins/HumanTest.cpp
               ${TestDir}/hominins/HumanTestingDiagnostics.cpp)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

add_subdirectory(${Dependencies}/sequoia/Source/sequoia Sequoia)
target_link_libraries(TestAll PRIVATE Sequoia)

sequoia_finalize_tests(TestAll ${TestDir} Tests)

