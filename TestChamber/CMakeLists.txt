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
               ${TestDir}/Core/DataStructures/StaticPartitionedSequenceTest.cpp
               ${TestDir}/Core/DataStructures/PartitionIteratorTest.cpp
               ${TestDir}/Core/DataStructures/PartitionedSequenceRegularTest.cpp
               ${TestDir}/Core/DataStructures/StaticLinearlyPartitionedSequenceTest.cpp
               ${TestDir}/Core/DataStructures/StaticLinearlyPartitionedSequenceTestingDiagnostics.cpp
)

sequoia_finalize_self(TestChamber ${TestDir}/Experimental)
