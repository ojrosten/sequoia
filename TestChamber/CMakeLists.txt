cmake_minimum_required(VERSION 3.15)

project(SequioaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

add_executable(TestChamber TestChamberMain.cpp)

include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/CompilerOptions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/TestFrameworkLibrary.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/TestIncludeDirectories.cmake)

ADD_TEST_INCLUDE_DIRS(TestChamber)

target_link_libraries(TestChamber PUBLIC TestFramework)

if(MSVC)
    target_link_libraries(TestChamber PUBLIC winmm)
endif()

target_compile_features(TestChamber PUBLIC cxx_std_20)
