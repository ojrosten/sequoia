cmake_minimum_required(VERSION 3.19)

project(SequioaTestAll DESCRIPTION "The Sequoia Library plus all tests"
                       LANGUAGES CXX)

add_executable(TestAll TestMain.cpp)

set(TestDir "../Tests")

target_sources(TestAll PRIVATE ${TestDir}/Experimental/ExperimentalTest.cpp
                               ${TestDir}/Core/DataStructures/StaticQueueTest.cpp
                               ${TestDir}/Core/DataStructures/PartitionedDataTest.cpp
                               ${TestDir}/Core/DataStructures/PartitionedDataAllocationTest.cpp
                               ${TestDir}/Core/DataStructures/StaticLinearlyPartitionedSequenceTest.cpp
                               ${TestDir}/Core/DataStructures/StaticQueueTestingDiagnostics.cpp
                               ${TestDir}/Core/DataStructures/PartitionedDataTestingDiagnostics.cpp
                               ${TestDir}/Core/DataStructures/StaticStackTest.cpp
                               ${TestDir}/Core/DataStructures/StaticLinearlyPartitionedSequenceTestingDiagnostics.cpp
                               ${TestDir}/Core/DataStructures/StaticStackTestingDiagnostics.cpp
                               ${TestDir}/Core/DataStructures/StaticPriorityQueueTest.cpp
                               ${TestDir}/Core/DataStructures/StaticPriorityQueueTestingDiagnostics.cpp
                               ${TestDir}/Core/Meta/TypeTraitsTest.cpp
                               ${TestDir}/Core/Meta/UtilitiesTest.cpp
                               ${TestDir}/Core/Meta/ConceptsTest.cpp
                               ${TestDir}/Core/Utilities/ArrayUtilitiesTest.cpp
                               ${TestDir}/Core/Utilities/UniformWrapperTest.cpp
                               ${TestDir}/Core/Utilities/IteratorTest.cpp
                               ${TestDir}/Core/Utilities/UniformWrapperTestingDiagnostics.cpp
                               ${TestDir}/Core/Concurrency/ConcurrencyModelsPerformanceTest.cpp
                               ${TestDir}/Core/Concurrency/ConcurrencyModelsTest.cpp
                               ${TestDir}/Core/Ownership/DataPoolAllocationTest.cpp
                               ${TestDir}/Core/Ownership/DataPoolTest.cpp
                               ${TestDir}/Runtime/FactoryTest.cpp
                               ${TestDir}/Runtime/FactoryTestingDiagnostics.cpp
                               ${TestDir}/Algorithms/AlgorithmsTest.cpp
                               ${TestDir}/Maths/Statistics/StatisticalAlgorithmsTest.cpp
                               ${TestDir}/Maths/Graph/NodeStorageTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphUnweightedTest.cpp
                               ${TestDir}/Maths/Graph/GraphMetaTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphEmbeddedTest.cpp
                               ${TestDir}/Maths/Graph/HeterogeneousNodeStorageTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphTestingDiagnostics.cpp
                               ${TestDir}/Maths/Graph/EdgeTest.cpp
                               ${TestDir}/Maths/Graph/HeterogeneousStaticGraphTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphFixedTopologyTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphTraversalsTest.cpp
                               ${TestDir}/Maths/Graph/NodeStorageAllocationTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphInitializationTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphUpdateTest.cpp
                               ${TestDir}/Maths/Graph/EdgeTestingDiagnostics.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphUnweightedAllocationTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphWeightedAllocationTest.cpp
                               ${TestDir}/Maths/Graph/StaticGraphManipulationsTest.cpp
                               ${TestDir}/Maths/Graph/StaticGraphTraversalsTest.cpp
                               ${TestDir}/Maths/Graph/DynamicSubgraphTest.cpp
                               ${TestDir}/Maths/Graph/DynamicGraphWeightedTest.cpp
                               ${TestDir}/Maths/Graph/StaticGraphInitializationTest.cpp
                               ${TestDir}/Maths/Sequences/MonotonicSequenceTest.cpp
                               ${TestDir}/Maths/Sequences/LinearSequenceTest.cpp
                               ${TestDir}/Maths/Sequences/LinearSequenceTestingDiagnostics.cpp
                               ${TestDir}/Maths/Sequences/MonotonicSequenceAllocationTest.cpp
                               ${TestDir}/Maths/Sequences/MonotonicSequenceTestingDiagnostics.cpp
                               ${TestDir}/Parsing/CommandLineArgumentsDiagnostics.cpp
                               ${TestDir}/Parsing/CommandLineArgumentsTest.cpp
                               ${TestDir}/TestFramework/MoveOnlyScopedAllocationTestDiagnostics.cpp
                               ${TestDir}/TestFramework/MoveOnlyAllocationTestDiagnostics.cpp
                               ${TestDir}/TestFramework/OrderableRegularTestDiagnostics.cpp
                               ${TestDir}/TestFramework/PerformanceTestDiagnostics.cpp
                               ${TestDir}/TestFramework/TestRunnerProjectCreation.cpp
                               ${TestDir}/TestFramework/MoveOnlyTestDiagnostics.cpp
                               ${TestDir}/TestFramework/CoreDiagnostics.cpp
                               ${TestDir}/TestFramework/OrderableMoveOnlyTestDiagnostics.cpp
                               ${TestDir}/TestFramework/TestRunnerDiagnostics.cpp
                               ${TestDir}/TestFramework/TestRunnerTest.cpp
                               ${TestDir}/TestFramework/RegularAllocationTestDiagnostics.cpp
                               ${TestDir}/TestFramework/TestRunnerDiagnosticsUtilities.cpp
                               ${TestDir}/TestFramework/TestRunnerTestCreation.cpp
                               ${TestDir}/TestFramework/FuzzyTestDiagnostics.cpp
                               ${TestDir}/TestFramework/TestRunnerEndToEndFreeTest.cpp
                               ${TestDir}/TestFramework/ScopedAllocationTestDiagnostics.cpp)

set(BUILD_SYS ${CMAKE_CURRENT_SOURCE_DIR}/../build_system)

file(GLOB_RECURSE HeaderFiles ${TestDir}/TestFramework/*.hpp)
target_sources(TestAll PRIVATE ${HeaderFiles})

include(${BUILD_SYS}/Utilities.cmake)

FINALIZE_SEQUOIA(TestAll)

