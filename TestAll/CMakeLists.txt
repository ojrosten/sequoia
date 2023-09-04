cmake_minimum_required(VERSION 3.20)

project(SequoiaTestAll DESCRIPTION "The Sequoia Library plus all tests"
                       LANGUAGES CXX)

include(../build_system/Utilities.cmake)

sequoia_init()

add_executable(TestAll TestMain.cpp)

set(TestDir ../Tests)

target_sources(TestAll PRIVATE
               ${TestDir}/Algorithms/AlgorithmsTest.cpp
               ${TestDir}/Core/Concurrency/ConcurrencyModelsPerformanceTest.cpp
               ${TestDir}/Core/Concurrency/ConcurrencyModelsTest.cpp
               ${TestDir}/Core/ContainerUtilities/ArrayUtilitiesTest.cpp
               ${TestDir}/Core/ContainerUtilities/IteratorTest.cpp
               ${TestDir}/Core/DataStructures/BucketedSequenceRegularTest.cpp
               ${TestDir}/Core/DataStructures/PartitionedDataAllocationTest.cpp
               ${TestDir}/Core/DataStructures/PartitionedDataTestingDiagnostics.cpp
               ${TestDir}/Core/DataStructures/PartitionedSequenceRegularTest.cpp
               ${TestDir}/Core/DataStructures/PartitionIteratorTest.cpp
               ${TestDir}/Core/DataStructures/StaticLinearlyPartitionedSequenceTest.cpp
               ${TestDir}/Core/DataStructures/StaticLinearlyPartitionedSequenceTestingDiagnostics.cpp
               ${TestDir}/Core/DataStructures/StaticPartitionedSequenceTest.cpp
               ${TestDir}/Core/DataStructures/StaticPriorityQueueTest.cpp
               ${TestDir}/Core/DataStructures/StaticPriorityQueueTestingDiagnostics.cpp
               ${TestDir}/Core/DataStructures/StaticQueueTest.cpp
               ${TestDir}/Core/DataStructures/StaticQueueTestingDiagnostics.cpp
               ${TestDir}/Core/DataStructures/StaticStackTest.cpp
               ${TestDir}/Core/DataStructures/StaticStackTestingDiagnostics.cpp
               ${TestDir}/Core/Logic/BitmaskFreeTest.cpp
               ${TestDir}/Core/Meta/ConceptsTest.cpp
               ${TestDir}/Core/Meta/TypeTraitsTest.cpp
               ${TestDir}/Core/Meta/UtilitiesTest.cpp
               ${TestDir}/Core/Object/CreatorFreeTest.cpp
               ${TestDir}/Core/Object/DataPoolAllocationTest.cpp
               ${TestDir}/Core/Object/DataPoolTest.cpp
               ${TestDir}/Core/Object/FaithfulWrapperTest.cpp
               ${TestDir}/Core/Object/FaithfulWrapperTestingDiagnostics.cpp
               ${TestDir}/Core/Object/SuiteFreeTest.cpp
               ${TestDir}/Experimental/ExperimentalTest.cpp
               ${TestDir}/Experimental/TypeListFreeTest.cpp
               ${TestDir}/Maths/Graph/Algorithms/DynamicGraphTraversalsTest.cpp
               ${TestDir}/Maths/Graph/Algorithms/DynamicGraphUpdateTest.cpp
               ${TestDir}/Maths/Graph/Algorithms/DynamicSubgraphTest.cpp
               ${TestDir}/Maths/Graph/Algorithms/GraphTraversalTestingUtilities.cpp
               ${TestDir}/Maths/Graph/Algorithms/StaticGraphTraversalsTest.cpp
               ${TestDir}/Maths/Graph/Components/Edges/EdgeTest.cpp
               ${TestDir}/Maths/Graph/Components/Edges/EdgeTestingDiagnostics.cpp
               ${TestDir}/Maths/Graph/Components/Meta/GraphMetaTest.cpp
               ${TestDir}/Maths/Graph/Components/Nodes/HeterogeneousNodeStorageTest.cpp
               ${TestDir}/Maths/Graph/Components/Nodes/NodeStorageAllocationTest.cpp
               ${TestDir}/Maths/Graph/Components/Nodes/NodeStorageTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Allocations/DynamicGraphUnweightedAllocationTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Allocations/DynamicGraphWeightedAllocationTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Allocations/DynamicGraphWeightedAllocationContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Diagnostics/DynamicGraphTestingDiagnostics.cpp
               ${TestDir}/Maths/Graph/Dynamic/Directed/DynamicDirectedGraphFundamentalWeightContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Directed/DynamicDirectedGraphFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Directed/DynamicDirectedGraphUnweightedContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Directed/DynamicDirectedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphSharedFundamentalWeightContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphSharedFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphSharedUnsortableWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphFundamentalWeightContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphMetaDataTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphUnsortableWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphUnweightedContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphSharedFundamentalWeightContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphSharedFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFundamentalWeightContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphMetaDataTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedContiguousTest.cpp
               ${TestDir}/Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/HeterogeneousStaticGraphTest.cpp
               ${TestDir}/Maths/Graph/Static/Directed/StaticDirectedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/Static/Directed/StaticDirectedGraphFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Static/Undirected/StaticUndirectedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/Static/Undirected/StaticUndirectedGraphFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Static/UndirectedEmbedded/StaticUndirectedEmbeddedGraphUnweightedTest.cpp
               ${TestDir}/Maths/Graph/Static/UndirectedEmbedded/StaticUndirectedEmbeddedGraphFundamentalWeightTest.cpp
               ${TestDir}/Maths/Graph/Tree/TreeTest.cpp
               ${TestDir}/Maths/Graph/Tree/TreeTestingDiagnostics.cpp
               ${TestDir}/Maths/Sequences/LinearSequenceTest.cpp
               ${TestDir}/Maths/Sequences/LinearSequenceTestingDiagnostics.cpp
               ${TestDir}/Maths/Sequences/MonotonicSequenceAllocationTest.cpp
               ${TestDir}/Maths/Sequences/MonotonicSequenceTest.cpp
               ${TestDir}/Maths/Sequences/MonotonicSequenceTestingDiagnostics.cpp
               ${TestDir}/Maths/Statistics/StatisticalAlgorithmsTest.cpp
               ${TestDir}/Parsing/CommandLineArgumentsDiagnostics.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTest.cpp
               ${TestDir}/Parsing/CommandLineArgumentsTestingUtilities.cpp
               ${TestDir}/Runtime/FactoryTest.cpp
               ${TestDir}/Runtime/FactoryTestingDiagnostics.cpp
               ${TestDir}/Runtime/ShellCommandsTest.cpp
               ${TestDir}/Runtime/ShellCommandsTestingDiagnostics.cpp
               ${TestDir}/Streaming/StreamingFreeTest.cpp
               ${TestDir}/TestFramework/AllocationTesting/MoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/MoveOnlyScopedAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/OrderableMoveOnlyAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/OrderableRegularAllocationTestDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnosticsBrokenSemantics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnosticsBrokenValueSemantics.cpp
               ${TestDir}/TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnosticsInefficientOperations.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnostics.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnosticsMixed.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnosticsThreeLevel.cpp
               ${TestDir}/TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnostics.cpp
               ${TestDir}/TestFramework/ChronoFreeDiagnostics.cpp
               ${TestDir}/TestFramework/CommandsFreeTest.cpp
               ${TestDir}/TestFramework/ComplexFreeDiagnostics.cpp
               ${TestDir}/TestFramework/ContainerFreeDiagnostics.cpp
               ${TestDir}/TestFramework/DependencyAnalyzerFreeTest.cpp
               ${TestDir}/TestFramework/ElementaryFreeDiagnostics.cpp
               ${TestDir}/TestFramework/ExceptionsFreeDiagnostics.cpp
               ${TestDir}/TestFramework/FailureInfoTest.cpp
               ${TestDir}/TestFramework/FailureInfoTestingDiagnostics.cpp
               ${TestDir}/TestFramework/FileSystemUtilitiesFreeTest.cpp
               ${TestDir}/TestFramework/FunctionFreeDiagnostics.cpp
               ${TestDir}/TestFramework/MaterialsUpdaterFreeTest.cpp
               ${TestDir}/TestFramework/MoveOnlyStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/MoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableMoveOnlyTestDiagnostics.cpp
               ${TestDir}/TestFramework/OrderableRegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/OutputFreeTest.cpp
               ${TestDir}/TestFramework/PathFreeDiagnostics.cpp
               ${TestDir}/TestFramework/PerformanceTestDiagnostics.cpp
               ${TestDir}/TestFramework/RegularStateTransitionDiagnostics.cpp
               ${TestDir}/TestFramework/RegularTestDiagnostics.cpp
               ${TestDir}/TestFramework/RelationalTestDiagnostics.cpp
               ${TestDir}/TestFramework/SmartPointerFreeDiagnostics.cpp
               ${TestDir}/TestFramework/StringFreeDiagnostics.cpp
               ${TestDir}/TestFramework/SumTypesFreeDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnostics.cpp
               ${TestDir}/TestFramework/TestRunnerDiagnosticsUtilities.cpp
               ${TestDir}/TestFramework/TestRunnerEndToEndFreeTest.cpp
               ${TestDir}/TestFramework/TestRunnerPerformanceTest.cpp
               ${TestDir}/TestFramework/TestRunnerProjectCreation.cpp
               ${TestDir}/TestFramework/TestRunnerTest.cpp
               ${TestDir}/TestFramework/TestRunnerTestCreation.cpp
               ${TestDir}/TextProcessing/IndentFreeTest.cpp
               ${TestDir}/TextProcessing/PatternsFreeTest.cpp
               ${TestDir}/TextProcessing/SubstitutionsFreeTest.cpp)

sequoia_finalize_self(TestAll ${TestDir})

