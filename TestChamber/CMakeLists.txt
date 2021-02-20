cmake_minimum_required(VERSION 3.15)

project(SequioaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

add_executable(TestChamber TestChamberMain.cpp)

include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/CompilerOptions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/TestFrameworkLibrary.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/Globbing.cmake)

HEADER_DIRECTORIES(testHeaderList ../Tests/*.hpp)
target_include_directories(TestChamber PUBLIC ${testHeaderList})
target_include_directories(TestChamber PUBLIC ../TestCommon)

target_link_libraries(TestChamber PUBLIC TestFramework)

if(MSVC)
    target_link_libraries(TestFrameworkDiagnostics PUBLIC winmm)
endif()

target_compile_features(TestChamber PUBLIC cxx_std_20)
