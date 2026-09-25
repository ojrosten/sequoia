cmake_minimum_required(VERSION 4.2)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a local test chamber"
                           LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../build_system/Utilities.cmake)

sequoia_init()

# Local/ is untracked, so each developer registers there the tests under their own focus. Each file
# of Template/ is copied into Local/ only where Local/ lacks it, so no configure discards a
# registration, and a file deleted from Local/ returns from the template. The copy must be newer
# than anything built from the file it replaces, or the build keeps the old object: configure_file
# writes a new file, whereas file(COPY) keeps the template's modification time.
sequoia_glob_files(TemplateFiles ${CMAKE_CURRENT_LIST_DIR}/Template *)
foreach(TemplateFile ${TemplateFiles})
    file(RELATIVE_PATH LocalFile ${CMAKE_CURRENT_LIST_DIR}/Template ${TemplateFile})
    if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/Local/${LocalFile})
        configure_file(${TemplateFile} ${CMAKE_CURRENT_LIST_DIR}/Local/${LocalFile} COPYONLY)
    endif()
endforeach()

add_executable(TestChamber Local/TestChamberMain.cpp)

set(TestDir ${CMAKE_CURRENT_LIST_DIR}/../Tests)

add_subdirectory(Local)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental Experimental)
