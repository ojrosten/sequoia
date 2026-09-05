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

        # C4702 fires on template code where a specialization makes a branch dead - a
        # container of compile-time length zero, say - and the optimizer attributes it to
        # the line of the *inlined* function rather than the dead call site, so the
        # reported location misleads.  This is a suppression rather than a fix, so it is
        # confined to the configurations that can raise it; unoptimized builds cannot.
        target_compile_options(${target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/wd4702>)
    else()
        target_compile_options(${target} PUBLIC -Wall -Wextra -Wpedantic -Wshadow)
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

FUNCTION(sequoia_set_run_target executable)
    add_custom_target(run 
        COMMAND $<TARGET_FILE:${executable}> ${EXEC_ARGS}
        DEPENDS ${executable}
    )
ENDFUNCTION()

# MSVC's address sanitizer is a *dynamic* runtime: the executable links against
# clang_rt.asan*.dll, which ships beside the compiler and is not on PATH unless the
# build happened to be launched from a shell that put it there. Absent the DLL the
# binary dies at startup with 0xc0000135 and no message worth reading - so an ASan
# build that works from the IDE fails from the 'run' target, or vice versa, purely
# on how it was invoked.
#
# The source directory is derived from CMAKE_CXX_COMPILER rather than from PATH, so
# it is correct by construction and cannot go stale as toolchains come and go.
#
# Whatever the toolchain ships is copied, rather than one named file, so that no
# mapping from configuration to runtime variant is encoded here. Measured with
# dumpbin on 14.51: a Debug build imports clang_rt.asan_dynamic-x86_64.dll - the
# plain one - even though it links the debug CRT (MSVCP140D, ucrtbased), so the
# obvious guess that /MDd pairs with the _dbg_ runtime is wrong, and a guess is not
# what a build should rest on. The files are ~2MB each and copy_if_different makes
# the steady state a no-op.
FUNCTION(sequoia_copy_asan_runtime target)
    if(MSVC AND CMAKE_CXX_FLAGS MATCHES "fsanitize=address")
        get_filename_component(toolchain_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
        file(GLOB asan_runtimes "${toolchain_bin}/clang_rt.asan*_dynamic-*.dll")
        if(NOT asan_runtimes)
            # Fatal rather than a warning: the symptom this function exists to
            # remove is a binary that links, builds and then dies at startup
            # with 0xC0000135 and nothing else to go on. A warning here buys a
            # green build and reinstates exactly that failure, one step later
            # and further from its cause.
            message(FATAL_ERROR
                "ASan requested but no clang_rt.asan*_dynamic-*.dll found in ${toolchain_bin}. "
                "${target} would link and then fail at startup with 0xC0000135 unless the "
                "runtime happens to be on PATH.")
        endif()
        foreach(runtime ${asan_runtimes})
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${runtime}" "$<TARGET_FILE_DIR:${target}>"
                VERBATIM)
        endforeach()
    endif()
ENDFUNCTION()

FUNCTION(sequoia_add_coverage_options target)
    if(CODE_COVERAGE)
        target_compile_options(${target} PRIVATE -coverage)
        target_link_options(${target} PRIVATE -coverage)
    endif()
ENDFUNCTION()

# Applied per target rather than through CMAKE_CXX_FLAGS so that CMake's own
# compiler checks never see it. Those compile to stdout, and -ftime-trace names
# its output after the output file, so a global flag drops a stray '-.json' in
# whichever directory cmake was invoked from - and puts the check's own traces
# in the build tree, where they are indistinguishable from translation units.
FUNCTION(sequoia_add_time_trace_options target)
    if(TIME_TRACE)
        target_compile_options(${target} PRIVATE -ftime-trace)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_finalize_tests target sourceGroupRoot sourceGroupPrefix)
    sequoia_compile_features(${target})
    sequoia_set_compile_options(${target})
    sequoia_set_properties(${target})
    sequoia_set_ide_source_groups_with_prefix(${target} ${sourceGroupRoot} ${sourceGroupPrefix})
    sequoia_add_coverage_options(${target})
    sequoia_add_time_trace_options(${target})
    if(CODE_COVERAGE)
        add_test(NAME ${target} COMMAND ${target} "--serial")
    else()
        add_test(NAME ${target} COMMAND ${target})
    endif()
    sequoia_set_run_target(${target})
    sequoia_copy_asan_runtime(${target})
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
    sequoia_add_time_trace_options(${target})
ENDFUNCTION()

FUNCTION(sequoia_finalize_executable target)
    sequoia_finalize_library(${target})
    sequoia_set_properties(${target})
    sequoia_set_run_target(${target})
    sequoia_copy_asan_runtime(${target})
ENDFUNCTION()
