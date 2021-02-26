cmake_minimum_required(VERSION 3.19)

project(MyProject LANGUAGES CXX)

add_executable(TestMain TestSandbox.cpp)

target_sources(TestMain PRIVATE
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Maybe/MaybeTestingDiagnostics.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Maybe/MaybeTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Utilities/IteratorTestingDiagnostics.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Utilities/IteratorTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/FooTestingDiagnostics.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/FooTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Partners/CoupleTestingDiagnostics.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Partners/CoupleTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Utilities/UtilitiesFreeTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/FooAllocationTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/ContainerTestingDiagnostics.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/ContainerTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/ContainerAllocationTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Stuff/ContainerPerformanceTest.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Utilities/VariadicTestingDiagnostics.cpp
               /Users/Claire/Dropbox/Sequoia/output/TestsTemporaryData/TestFramework/TestRunnerTestCreation/WorkingCopy/FakeProject/Tests/Utilities/VariadicTest.cpp)

SET(BUILD_SYS SEQUOIA_ROOT/build_system)

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE(TestMain)

