set(CurrentDir ${CMAKE_CURRENT_LIST_DIR})

FUNCTION(sequoia_init)
	if(NOT MSVC)
		find_package(Threads REQUIRED)
	endif()
ENDFUNCTION()

FUNCTION(sequoia_link_libraries target)
    target_link_libraries(${target} PUBLIC TestFramework)

    if(MSVC)
        target_link_libraries(${target} PRIVATE winmm)
        set_target_properties(${target} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
	else()
		target_link_libraries(${target} PRIVATE Threads::Threads)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_compile_options)
    if (MSVC)
        add_definitions(/W4)
        add_definitions(/bigobj)
        add_definitions(/MP)
    else()
        add_compile_options(-Wall -Wextra -pedantic)
    endif()
ENDFUNCTION()

FUNCTION(sequoia_finalize target headerDirForIDE)
    sequoia_compile_options()

    add_subdirectory(${CurrentDir}/../Source TestFramework)

    sequoia_link_libraries(${target})
    target_compile_features(${target} PUBLIC cxx_std_23)
	
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target})
	endif()
	
	if(headerDirForIDE)
		file(GLOB_RECURSE HeaderFiles ${headerDirForIDE}/*.hpp)
		target_sources(${target} PRIVATE ${HeaderFiles})
	endif()
ENDFUNCTION()

FUNCTION(sequoia_finalize_self target headersForIDE)
    target_include_directories(${target} PRIVATE ${CurrentDir}/../TestCommon)
    target_include_directories(${target} PRIVATE ${CurrentDir}/../Tests)

    sequoia_finalize(${target} ${headersForIDE})
ENDFUNCTION()