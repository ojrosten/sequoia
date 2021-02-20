cmake_minimum_required(VERSION 3.15)

project(SequioaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

add_executable(TestChamber TestChamberMain.cpp)

SET(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

include(${BUILD_SYS}/CompilerOptions.cmake)
include(${BUILD_SYS}/TestFrameworkLibrary.cmake)
include(${BUILD_SYS}/TestIncludeDirectories.cmake)
include(${BUILD_SYS}/LinkToLibraries.cmake)

ADD_TEST_INCLUDE_DIRS(TestChamber)
LINK_LIBRARIES(TestChamber)

target_compile_features(TestChamber PUBLIC cxx_std_20)
