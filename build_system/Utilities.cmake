set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})

FUNCTION(LINK_LIBRARIES target)
    target_link_libraries(${target} PUBLIC TestFramework)

    if(MSVC)
        target_link_libraries(${target} PRIVATE winmm)		
        set_target_properties(${target} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
    endif()
ENDFUNCTION()

FUNCTION(COMPILE_OPTIONS)
    if (MSVC)
        add_definitions(/W4)
        add_definitions(/bigobj)
        add_definitions(/MP)
    else()
        add_compile_options(-Wall -Wextra -pedantic)
    endif()
ENDFUNCTION()

FUNCTION(FINALIZE target)
    COMPILE_OPTIONS()

    add_subdirectory(${CURRENT_DIR}/../Source TestFramework)

    LINK_LIBRARIES(${target})
    target_compile_features(${target} PUBLIC cxx_std_20)
ENDFUNCTION()

FUNCTION(FINALIZE_SEQUOIA target)
    target_include_directories(${target} PRIVATE ${CURRENT_DIR}/../TestCommon)
    target_include_directories(${target} PRIVATE ${CURRENT_DIR}/../Tests)

    FINALIZE(${target})
ENDFUNCTION()