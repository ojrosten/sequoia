cmake_minimum_required(VERSION 3.20)

project(SequoiaTestChamber DESCRIPTION "The Sequoia Library plus a test chamber"
                           LANGUAGES CXX)

include(../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestChamber TestChamberMain.cpp)

set(TestDir ../Tests)

target_sources(TestChamber PRIVATE 
               ${TestDir}/Experimental/ExperimentalTest.cpp
               ${TestDir}/Core/DataStructures/BucketedSequenceRegularTest.cpp
               ${TestDir}/Core/DataStructures/PartitionedDataAllocationTest.cpp
               ${TestDir}/Core/DataStructures/PartitionedDataTestingDiagnostics.cpp
               ${TestDir}/Core/DataStructures/PartitionedDataTest.cpp
               ${TestDir}/Core/DataStructures/PartitionedSequenceRegularTest.cpp
)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental)
