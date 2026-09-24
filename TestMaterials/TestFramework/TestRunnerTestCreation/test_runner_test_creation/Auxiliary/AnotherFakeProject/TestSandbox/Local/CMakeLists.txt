#!add_subdirectory(${SourceDir}/curlew MyProject)
#!target_link_libraries(TestAll PUBLIC MyProject)

target_sources(TestAll PRIVATE)
