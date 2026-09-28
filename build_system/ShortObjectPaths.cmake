# Included at the start of project() on the Windows presets, through CMAKE_PROJECT_INCLUDE_BEFORE.
#
# cl.exe cannot write a file whose full path reaches MAX_PATH, 260 characters, whatever the
# registry says: C1083 on the object, C1041 on the .pdb. Under Ninja, CMake places an object at
# CMakeFiles/<target>.dir/<source path>.obj, where a source outside the project directory
# contributes its full path, and a probe's at
# CMakeFiles/CMakeScratch/TryCompile-xxxxxx/CMakeFiles/cmTC_xxxxx.dir/CMakeCXXCompilerABI.cpp.obj,
# 95 characters below the build directory, which on the CI runner puts a probe in the projects the
# tests nest at 263. CMake's SHORT strategy names the directory and the object by
# hash, .o/xxxxxxxx/xxxxxxxx.obj. prune reads each object's source from build.ninja, not from the
# object's name, so the Ninja half is unaffected; for the tracker's logs see BuildArtefacts.cpp's
# object_of, which refuses an entry it cannot attribute rather than guessing.
#
# CMake still measures the long name it no longer writes against CMAKE_OBJECT_PATH_MAX, 250 on
# Windows - seen in 4.4.3, not checked below it - and the warning lands in the configure output a
# test captures, whose prediction has no warning. The measure includes the leading ./ the Ninja
# generator writes, so 261 trips at a real path of 260, where cl fails; the check on the long name
# trips fifteen characters earlier for a probe (CMakeCXXCompilerABI.cpp.obj against xxxxxxxx.obj).
#
# Under the Visual Studio generator the objects gain the same margin; the tracker's logs do not,
# their directory still being named after the target, so UntrackedTryCompiles.cmake is needed
# beside this file.
#
# A probe configures with a cache of its own, so both variables are relayed into it. The strategy
# is read from the cache alone: a plain set() is ignored, and a cache entry already there is kept,
# which is why a tree configured before this file joined the preset says so rather than silently
# building the way it always did.
set(CMAKE_INTERMEDIATE_DIR_STRATEGY SHORT CACHE STRING "Object paths hashed to stay under MAX_PATH: see ShortObjectPaths.cmake")
set(CMAKE_OBJECT_PATH_MAX 261)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_INTERMEDIATE_DIR_STRATEGY CMAKE_OBJECT_PATH_MAX)

if(NOT CMAKE_INTERMEDIATE_DIR_STRATEGY STREQUAL "SHORT")
  message(FATAL_ERROR "This build tree caches CMAKE_INTERMEDIATE_DIR_STRATEGY as '${CMAKE_INTERMEDIATE_DIR_STRATEGY}', so its object paths are the long ones: delete the tree and configure again")
endif()
