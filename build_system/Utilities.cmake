set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})

include(${CURRENT_DIR}/Globbing.cmake)

FUNCTION(ADD_TEST_INCLUDE_DIRS target)
    HEADER_DIRECTORIES(testHeaderList ../Tests/*.hpp)
    target_include_directories(${target} PUBLIC ${testHeaderList})
    target_include_directories(${target} PUBLIC ../TestCommon)
ENDFUNCTION()

FUNCTION(LINK_LIBRARIES target)
    target_link_libraries(${target} PUBLIC TestFramework)

    if(MSVC)
        target_link_libraries(${target} PRIVATE winmm)
    endif()
ENDFUNCTION()

FUNCTION(FINALIZE target)
    include(${CURRENT_DIR}/CompilerOptions.cmake)
    include(${CURRENT_DIR}/TestFrameworkLibrary.cmake)

    ADD_TEST_INCLUDE_DIRS(${target})
    LINK_LIBRARIES(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)
ENDFUNCTION()