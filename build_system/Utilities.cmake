FUNCTION(sequoia_compile_options)
    if (MSVC)
        add_definitions(/W4)
        add_definitions(/bigobj)
        add_definitions(/MP)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic)
        if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
            add_compile_options(-Wno-missing-braces -Wno-unused-lambda-capture)
        endif()         
    endif()
ENDFUNCTION()

FUNCTION(sequoia_init)
    sequoia_compile_options()
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

FUNCTION(set_headers_for_ide target directory)
	file(GLOB_RECURSE HeaderFiles ${directory}/*.h*)
	target_sources(${target} PRIVATE ${HeaderFiles})
ENDFUNCTION()

FUNCTION(sequoia_compile_features target)
	target_compile_features(${target} PUBLIC cxx_std_23)
ENDFUNCTION()

FUNCTION(sequoia_finalize target headerDirForIDE) 
	sequoia_compile_features(${target})

    add_subdirectory(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Source TestFramework)

    sequoia_link_libraries(${target})
	
	if (MSVC)
		set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${target})
	endif()
	
	set_headers_for_ide(${target} ${headerDirForIDE})
ENDFUNCTION()

FUNCTION(sequoia_finalize_self target headersForIDE)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../TestCommon)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../Tests)

    sequoia_finalize(${target} ${headersForIDE})
ENDFUNCTION()

FUNCTION(sequoia_finalize_library target)
	sequoia_compile_features(${target})

	set_headers_for_ide(${target} ${CMAKE_CURRENT_LIST_DIR})
ENDFUNCTION()