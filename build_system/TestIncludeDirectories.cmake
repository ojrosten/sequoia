include(${CMAKE_CURRENT_SOURCE_DIR}/../build_system/Globbing.cmake)

FUNCTION(ADD_TEST_INCLUDE_DIRS target)
    HEADER_DIRECTORIES(testHeaderList ../Tests/*.hpp)
    target_include_directories(${target} PUBLIC ${testHeaderList})
    target_include_directories(${target} PUBLIC ../TestCommon)
ENDFUNCTION()