cmake_minimum_required(VERSION 4.2)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a local test chamber"
                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

# Local/ is untracked, so each developer registers there the tests under their own focus.
# It is seeded from Template/ only when absent, so no configure discards those registrations;
# delete it to start again from the template.
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/Local)
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/Template/ DESTINATION ${CMAKE_CURRENT_LIST_DIR}/Local)
endif()

add_executable(TestChamber Local/TestChamberMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

add_subdirectory(Local)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental Experimental)
