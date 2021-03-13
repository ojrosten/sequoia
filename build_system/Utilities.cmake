set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})

FUNCTION(LINK_LIBRARIES target)
    target_link_libraries(${target} PUBLIC TestFramework)

    if(MSVC)
        target_link_libraries(${target} PRIVATE winmm)
    endif()
ENDFUNCTION()

FUNCTION(FINALIZE target)
    include(${CURRENT_DIR}/CompilerOptions.cmake)
    add_subdirectory(${CURRENT_DIR}/TestFramework TestFramework)

    LINK_LIBRARIES(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)
ENDFUNCTION()

FUNCTION(FINALIZE_SEQUOIA target)
    target_include_directories(${target} PRIVATE ../TestCommon)
    target_include_directories(${target} PRIVATE ../Tests)

    FINALIZE(${target})
ENDFUNCTION()