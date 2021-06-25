cmake_minimum_required(VERSION 3.19)

project(MyProjectTests LANGUAGES CXX)

add_executable(TestAll TestAllMain.cpp)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BuildSystem ../build_system)
set(SourceDir ${CURRENT_DIR}/../Source)
set(TestDir ${CURRENT_DIR}/../Tests)

#!add_subdirectory(${SourceDir} MyProject)
#!target_link_libraries(TestAll PUBLIC MyProject)

target_sources(TestAll PRIVATE)

file(GLOB_RECURSE HeaderFiles ${TestDir}/*.hpp)
target_sources(TestAll PRIVATE ${HeaderFiles})

include(${BuildSystem}/Utilities.cmake)

target_include_directories(TestAll PRIVATE ${SourceDir})
target_include_directories(TestAll PRIVATE ${TestDir})

if (MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT TestAll)
endif()

FINALIZE(TestAll)

