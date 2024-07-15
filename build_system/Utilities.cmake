FUNCTION(sequoia_compile_options)
    if (MSVC)
        add_definitions(/W4)
        add_definitions(/bigobj)
        add_definitions(/MP)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(-Wno-missing-braces -Wno-unused-lambda-capture -Wno-braced-scalar-init -Wno-unused-function)
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            add_compile_options(-Wno-comment)
        endif()
    endif()
ENDFUNCTION()

FUNCTION(sequoia_init)
    sequoia_compile_options()
    if(NOT WIN32)
        find_package(Threads REQUIRED)
        find_package(TBB REQUIRED)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_link_libraries target)
    if(WIN32)
        target_link_libraries(${target} PUBLIC winmm)
    else()
        target_link_libraries(${target} PUBLIC Threads::Threads)
        target_link_libraries(${target} PUBLIC TBB::tbb)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_set_ide_source_groups target directory)
    if (MSVC)
        file(GLOB_RECURSE HeaderFiles ${directory}/*.h*)
        source_group(TREE ${directory} FILES ${HeaderFiles})
        target_sources(${target} PRIVATE ${HeaderFiles})

        file(GLOB_RECURSE SourceFiles ${directory}/*.c*)
        source_group(TREE ${directory} FILES ${SourceFiles})
    endif()
ENDFUNCTION()

FUNCTION(sequoia_set_ide_source_groups_with_prefix target directory sourceGroupPrefix)
    if (MSVC)
        file(GLOB_RECURSE HeaderFiles ${directory}/*.h*)
        source_group(TREE ${directory} PREFIX ${sourceGroupPrefix} FILES ${HeaderFiles})
        target_sources(${target} PRIVATE ${HeaderFiles})

        file(GLOB_RECURSE SourceFiles ${directory}/*.c*)
        source_group(TREE ${directory} PREFIX ${sourceGroupPrefix} FILES ${SourceFiles})
    endif()
ENDFUNCTION()

FUNCTION(sequoia_compile_features target)
    target_compile_features(${target} PUBLIC cxx_std_23)
ENDFUNCTION()

FUNCTION(sequoia_set_properties target)
    if (MSVC)
        set_target_properties(${target} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target})
        target_sources(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../resources/win/longpaths.manifest)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_finalize target sourceGroupRoot sourceGroupPrefix)
    sequoia_compile_features(${target})

    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Source Sequoia)
    target_link_libraries(${target} PUBLIC Sequoia)

    sequoia_set_properties(${target})
    sequoia_set_ide_source_groups_with_prefix(${target} ${sourceGroupRoot} ${sourceGroupPrefix})
ENDFUNCTION()

FUNCTION(sequoia_finalize_self target sourceGroupRoot sourceGroupPrefix)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../TestCommon)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Tests)

    sequoia_finalize(${target} ${sourceGroupRoot} ${sourceGroupPrefix})
ENDFUNCTION()

FUNCTION(sequoia_finalize_library target)
    sequoia_compile_features(${target})
    sequoia_link_libraries(${target})
    sequoia_set_ide_source_groups(${target} ${CMAKE_CURRENT_LIST_DIR}/${target})
ENDFUNCTION()

FUNCTION(sequoia_finalize_executable target)
    sequoia_compile_features(${target})
    sequoia_link_libraries(${target})
    sequoia_set_properties(${target})
    sequoia_set_ide_source_groups(${target} ${CMAKE_CURRENT_LIST_DIR})
endfunction()