set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})

FUNCTION(LINK_LIBRARIES target)
    target_link_libraries(${target} PUBLIC TestFramework)

    if(MSVC)
        target_link_libraries(${target} PRIVATE winmm)
    endif()
ENDFUNCTION()

FUNCTION(COMPILE_OPTIONS)
    if (MSVC)
       add_compile_options(/W4)
       add_definitions(/bigobj)
    else()
        add_compile_options(-Wall -Wextra -pedantic)
    endif()
ENDFUNCTION()

FUNCTION(FINALIZE target)
    COMPILE_OPTIONS()

    add_subdirectory(../Source TestFramework)

    LINK_LIBRARIES(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)
ENDFUNCTION()

FUNCTION(FINALIZE_SEQUOIA target)
    target_include_directories(${target} PRIVATE ../TestCommon)
    target_include_directories(${target} PRIVATE ../Tests)

    FINALIZE(${target})
ENDFUNCTION()