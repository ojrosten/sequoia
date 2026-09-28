# Included at the end of project() under the Visual Studio generator, through
# CMAKE_PROJECT_INCLUDE in the presets: the compiler probes ran untracked
# (UntrackedTryCompiles.cmake); everything from here is tracked, since prune reads the logs.
#
# ShortObjectPaths.cmake names each object by hash, so every source carries an ObjectFileName of
# its own. cl.exe takes one /Fo per invocation, so MSBuild then runs one cl.exe per source and /MP
# has nothing to share out: a clean Debug build of TestAll on the Windows laptop took 23.7 minutes,
# one compiler at a time, against 5.1 with the object paths unhashed. MSBuild's MultiToolTask
# schedules each source as a process of its own, whatever its ObjectFileName, and took 5.5 minutes.
# EnforceProcessCountAcrossBuilds caps the compilers across every project MSBuild builds at once,
# where the default caps each project separately. Were the object paths no longer hashed, /MP alone
# would parallelise the build again.
#
# MultiToolTask prefixes the names of the compiler's logs with Microsoft.Build.CPPTasks.; prune's
# reader, is_tlog in BuildArtefacts.cpp, takes either name.
#
# The globals are appended to any CMAKE_VS_GLOBALS in the cache, so that globals given on the
# command line or by a preset still reach the project's targets. A try_compile after project()
# receives them too, UntrackedTryCompiles.cmake having relayed the variable into every try_compile.
set(CMAKE_VS_GLOBALS $CACHE{CMAKE_VS_GLOBALS} UseMultiToolTask=true EnforceProcessCountAcrossBuilds=true)
