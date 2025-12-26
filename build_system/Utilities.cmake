# Hack to suppress the CTest targets appearing in IDEs
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

option(CODE_COVERAGE "Build with Code Coverage" OFF)
set(EXEC_ARGS "" CACHE STRING "Command-line arguments for the 'run' target.")

FUNCTION(sequoia_init)
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
        file(GLOB_RECURSE HeaderFiles ${directory}/*.h*)
        source_group(TREE ${directory} FILES ${HeaderFiles})
        target_sources(${target} PRIVATE ${HeaderFiles})

        file(GLOB_RECURSE SourceFiles ${directory}/*.c*)
        source_group(TREE ${directory} FILES ${SourceFiles})
ENDFUNCTION()

FUNCTION(sequoia_set_ide_source_groups_with_prefix target directory sourceGroupPrefix)
        file(GLOB_RECURSE HeaderFiles ${directory}/*.h*)
        source_group(TREE ${directory} PREFIX ${sourceGroupPrefix} FILES ${HeaderFiles})
        target_sources(${target} PRIVATE ${HeaderFiles})

        file(GLOB_RECURSE SourceFiles ${directory}/*.c*)
        source_group(TREE ${directory} PREFIX ${sourceGroupPrefix} FILES ${SourceFiles})
ENDFUNCTION()

FUNCTION(sequoia_compile_features target)
    if(WIN32)
        target_compile_features(${target} PUBLIC cxx_std_23)
    else()
        target_compile_features(${target} PUBLIC cxx_std_26)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_set_compile_options target)
    if (MSVC)
        target_compile_options(${target} PUBLIC /W4)
        target_compile_options(${target} PUBLIC /bigobj)
        target_compile_options(${target} PUBLIC /MP)
    else()
        target_compile_options(${target} PUBLIC -Wall -Wextra -Wpedantic)
    endif()

    target_compile_options(${target} PRIVATE ${WARNING_SUPPRESSIONS})
ENDFUNCTION()

FUNCTION(sequoia_set_properties target)
    if (MSVC)
        set_target_properties(${target} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target})
        target_sources(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../resources/win/longpaths.manifest)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_set_run_target exectuable)
    add_custom_target(run 
        COMMAND $<TARGET_FILE:${exectuable}> ${EXEC_ARGS}
        DEPENDS ${exectuable}
    )
ENDFUNCTION()

FUNCTION(sequoia_add_coverage_options target)
    if(CODE_COVERAGE)
        target_compile_options(${target} PRIVATE -coverage)
        target_link_options(${target} PRIVATE -coverage)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_finalize_tests target sourceGroupRoot sourceGroupPrefix)
    sequoia_compile_features(${target})
    sequoia_set_compile_options(${target})
    sequoia_set_properties(${target})
    sequoia_set_ide_source_groups_with_prefix(${target} ${sourceGroupRoot} ${sourceGroupPrefix})
    sequoia_add_coverage_options(${target})
    if(CODE_COVERAGE)
        add_test(NAME ${target} COMMAND ${target} "--serial")
    endif()
    sequoia_set_run_target(${target})
ENDFUNCTION()

FUNCTION(sequoia_finalize_self target sourceGroupRoot sourceGroupPrefix)
    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Source/sequoia sequoia)
    target_link_libraries(${target} PUBLIC sequoia)

    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../TestCommon)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Tests)

    sequoia_finalize_tests(${target} ${sourceGroupRoot} ${sourceGroupPrefix})
ENDFUNCTION()

FUNCTION(sequoia_finalize_library target)
    sequoia_compile_features(${target})
    sequoia_set_compile_options(${target})
    sequoia_link_libraries(${target})
    sequoia_set_ide_source_groups(${target} ${CMAKE_CURRENT_LIST_DIR})
    sequoia_add_coverage_options(${target})
ENDFUNCTION()

FUNCTION(sequoia_finalize_executable target)
    sequoia_finalize_library(${target})
    sequoia_set_properties(${target})
    sequoia_set_run_target(${target})
ENDFUNCTION()
