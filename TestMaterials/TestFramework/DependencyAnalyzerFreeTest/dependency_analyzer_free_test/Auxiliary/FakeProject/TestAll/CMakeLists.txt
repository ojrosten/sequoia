cmake_minimum_required(VERSION 4.2)

project(FakeProjectTests LANGUAGES CXX)

set(Dependencies ${CMAKE_CURRENT_LIST_DIR}/../dependencies)
set(BuildSystem ${Dependencies}/sequoia/build_system)
set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)
set(SourceDir ${CMAKE_CURRENT_LIST_DIR}/../Source)
set(TestUtilitiesDir ${CMAKE_CURRENT_LIST_DIR}/../TestUtilities)

include(${BuildSystem}/Utilities.cmake)

sequoia_init()

add_executable(TestAll TestAllMain.cpp)

add_subdirectory(${SourceDir}/fakeProject FakeProject)
target_link_libraries(TestAll PUBLIC FakeProject)

target_sources(TestAll PRIVATE
               ${TestDir}/Cycle/FirstFreeTest.cpp
               ${TestDir}/Cycle/SecondFreeTest.cpp
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
               ${TestDir}/Utilities/Thing/UniqueThingTest.cpp
               ${TestDir}/Utilities/Thing/UniqueThingTestingDiagnostics.cpp
               ${TestDir}/Utilities/UsefulThingsFreeTest.cpp
               ${TestDir}/Utilities/UtilitiesFreeTest.cpp
               ${TestUtilitiesDir}/myLib/Utils.cpp)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})
target_include_directories(TestAll PRIVATE ${TestUtilitiesDir})

add_subdirectory(${Dependencies}/foo/Source/foo foo)

add_subdirectory(${Dependencies}/sequoia/Source/sequoia sequoia)
target_link_libraries(TestAll PRIVATE sequoia)

sequoia_finalize_tests(TestAll ${TestDir} Tests)
