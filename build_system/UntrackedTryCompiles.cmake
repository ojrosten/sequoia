# Included at the start of project() under the Visual Studio generator, through
# CMAKE_PROJECT_INCLUDE_BEFORE in the presets; TrackedBuild.cmake replaces the CMAKE_VS_GLOBALS this
# file sets at the end.
#
# MSBuild's FileTracker writes a log per tool run beneath the try-compile's scratch
# directory - CMakeFiles/CMakeScratch/TryCompile-xxxxxx/cmTC_xxxxx.dir/Debug/cmTC_xxxxx.tlog/,
# about a hundred characters below the build directory - and cannot write past MAX_PATH
# whatever the registry says (FTK1011). A build tree deeper than about 155 characters then
# fails "Detecting CXX compiler ABI info" for no reason of the compiler's, which sequoia's
# tests reach through the projects they nest. A try-compile needs no tracking, its result
# being pass or fail, so the compiler probes run without it; the project's own build keeps
# its logs, which prune reads.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_VS_GLOBALS)
set(CMAKE_VS_GLOBALS TrackFileAccess=false)
