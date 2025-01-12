////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Algorithms/AlgorithmsTest.hpp"
#include "Core/Concurrency/ConcurrencyModelsPerformanceTest.hpp"
#include "Core/Concurrency/ConcurrencyModelsTest.hpp"
#include "Core/ContainerUtilities/ArrayUtilitiesTest.hpp"
#include "Core/ContainerUtilities/IteratorTest.hpp"
#include "Core/DataStructures/BucketedSequenceAllocationTest.hpp"
#include "Core/DataStructures/BucketedSequenceRegularTest.hpp"
#include "Core/DataStructures/PartitionIteratorTest.hpp"
#include "Core/DataStructures/PartitionedDataTestingDiagnostics.hpp"
#include "Core/DataStructures/PartitionedSequenceAllocationTest.hpp"
#include "Core/DataStructures/PartitionedSequenceRegularTest.hpp"
#include "Core/DataStructures/StaticLinearlyPartitionedSequenceTest.hpp"
#include "Core/DataStructures/StaticLinearlyPartitionedSequenceTestingDiagnostics.hpp"
#include "Core/DataStructures/StaticPartitionedSequenceTest.hpp"
#include "Core/DataStructures/StaticPriorityQueueTest.hpp"
#include "Core/DataStructures/StaticPriorityQueueTestingDiagnostics.hpp"
#include "Core/DataStructures/StaticQueueTest.hpp"
#include "Core/DataStructures/StaticQueueTestingDiagnostics.hpp"
#include "Core/DataStructures/StaticStackTest.hpp"
#include "Core/DataStructures/StaticStackTestingDiagnostics.hpp"
#include "Core/Logic/BitmaskFreeTest.hpp"
#include "Core/Meta/ConceptsTest.hpp"
#include "Core/Meta/SequencesFreeTest.hpp"
#include "Core/Meta/TypeListFreeTest.hpp"
#include "Core/Meta/TypeTraitsTest.hpp"
#include "Core/Meta/UtilitiesTest.hpp"
#include "Core/Object/CreatorFreeTest.hpp"
#include "Core/Object/SuiteFreeTest.hpp"
#include "Experimental/ExperimentalTest.hpp"
#include "Experimental/FlattenTypeListFreeTest.hpp"
#include "FileSystem/NormalPathTest.hpp"
#include "FileSystem/NormalPathTestingDiagnostics.hpp"
#include "Maths/Graph/Algorithms/DynamicGraphTraversalsTest.hpp"
#include "Maths/Graph/Algorithms/DynamicGraphUpdateTest.hpp"
#include "Maths/Graph/Algorithms/DynamicSubgraphTest.hpp"
#include "Maths/Graph/Algorithms/StaticGraphTraversalsTest.hpp"
#include "Maths/Graph/Components/Edges/EdgeTest.hpp"
#include "Maths/Graph/Components/Edges/EdgeTestingDiagnostics.hpp"
#include "Maths/Graph/Components/Meta/GraphMetaTest.hpp"
#include "Maths/Graph/Components/Nodes/HeterogeneousNodeStorageTest.hpp"
#include "Maths/Graph/Components/Nodes/NodeStorageAllocationTest.hpp"
#include "Maths/Graph/Components/Nodes/NodeStorageTest.hpp"
#include "Maths/Graph/Dynamic/Allocations/DynamicGraphUnweightedAllocationBucketedTest.hpp"
#include "Maths/Graph/Dynamic/Allocations/DynamicGraphUnweightedAllocationContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Allocations/DynamicGraphWeightedAllocationBucketedTest.hpp"
#include "Maths/Graph/Dynamic/Allocations/DynamicGraphWeightedAllocationContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Diagnostics/DynamicGraphTestingDiagnostics.hpp"
#include "Maths/Graph/Dynamic/Directed/DynamicDirectedGraphFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Directed/DynamicDirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Dynamic/Directed/DynamicDirectedGraphUnweightedContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Directed/DynamicDirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphMetaDataTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphSharedFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphSharedFundamentalWeightTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphSharedUnsortableWeightTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphUnsortableWeightTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphUnweightedContiguousTest.hpp"
#include "Maths/Graph/Dynamic/Undirected/DynamicUndirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphMetaDataTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphSharedFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphSharedFundamentalWeightTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedContiguousTest.hpp"
#include "Maths/Graph/Dynamic/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedTest.hpp"
#include "Maths/Graph/HeterogeneousStaticGraphTest.hpp"
#include "Maths/Graph/Static/Directed/StaticDirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Static/Directed/StaticDirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/Static/Undirected/StaticUndirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Static/Undirected/StaticUndirectedGraphUnsortableWeightTest.hpp"
#include "Maths/Graph/Static/Undirected/StaticUndirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/Static/UndirectedEmbedded/StaticUndirectedEmbeddedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Static/UndirectedEmbedded/StaticUndirectedEmbeddedGraphUnweightedTest.hpp"
#include "Maths/Graph/Tree/TreeTest.hpp"
#include "Maths/Graph/Tree/TreeTestingDiagnostics.hpp"
#include "Maths/Sequences/LinearSequenceTest.hpp"
#include "Maths/Sequences/LinearSequenceTestingDiagnostics.hpp"
#include "Maths/Sequences/MonotonicSequenceAllocationTest.hpp"
#include "Maths/Sequences/MonotonicSequenceTest.hpp"
#include "Maths/Sequences/MonotonicSequenceTestingDiagnostics.hpp"
#include "Maths/Statistics/StatisticalAlgorithmsTest.hpp"
#include "Parsing/CommandLineArgumentsDiagnostics.hpp"
#include "Parsing/CommandLineArgumentsTest.hpp"
#include "Runtime/FactoryTest.hpp"
#include "Runtime/FactoryTestingDiagnostics.hpp"
#include "Runtime/ShellCommandsTest.hpp"
#include "Runtime/ShellCommandsTestingDiagnostics.hpp"
#include "Streaming/StreamingFreeTest.hpp"
#include "TestFramework/AllocationTesting/MoveOnlyAllocationTestDiagnostics.hpp"
#include "TestFramework/AllocationTesting/MoveOnlyScopedAllocationTestDiagnostics.hpp"
#include "TestFramework/AllocationTesting/OrderableMoveOnlyAllocationTestDiagnostics.hpp"
#include "TestFramework/AllocationTesting/OrderableRegularAllocationTestDiagnostics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnostics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnosticsBrokenSemantics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnosticsBrokenValueSemantics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalseNegativeDiagnosticsInefficientOperations.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnostics.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnostics.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnostics.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnosticsMixed.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnosticsThreeLevel.hpp"
#include "TestFramework/BasicTestInterfaceFreeTest.hpp"
#include "TestFramework/ChronoFreeDiagnostics.hpp"
#include "TestFramework/CommandsFreeTest.hpp"
#include "TestFramework/ComplexFreeDiagnostics.hpp"
#include "TestFramework/ContainerFreeDiagnostics.hpp"
#include "TestFramework/DependencyAnalyzerFreeTest.hpp"
#include "TestFramework/ElementaryFreeDiagnostics.hpp"
#include "TestFramework/ExceptionsFreeDiagnostics.hpp"
#include "TestFramework/FailureInfoTest.hpp"
#include "TestFramework/FailureInfoTestingDiagnostics.hpp"
#include "TestFramework/FileSystemUtilitiesFreeTest.hpp"
#include "TestFramework/FreeCheckersMetaFreeTest.hpp"
#include "TestFramework/FunctionFreeDiagnostics.hpp"
#include "TestFramework/IndividualTestPathsFreeTest.hpp"
#include "TestFramework/MaterialsUpdaterFreeTest.hpp"
#include "TestFramework/MoveOnlyStateTransitionDiagnostics.hpp"
#include "TestFramework/MoveOnlyTestDiagnostics.hpp"
#include "TestFramework/OrderableMoveOnlyTestDiagnostics.hpp"
#include "TestFramework/OrderableRegularTestDiagnostics.hpp"
#include "TestFramework/OutputFreeTest.hpp"
#include "TestFramework/PathFreeDiagnostics.hpp"
#include "TestFramework/PerformanceTestDiagnostics.hpp"
#include "TestFramework/RegularStateTransitionDiagnostics.hpp"
#include "TestFramework/RegularTestDiagnostics.hpp"
#include "TestFramework/RelationalTestDiagnostics.hpp"
#include "TestFramework/SmartPointerFreeDiagnostics.hpp"
#include "TestFramework/StringFreeDiagnostics.hpp"
#include "TestFramework/SumTypesFreeDiagnostics.hpp"
#include "TestFramework/TestRunnerDiagnostics.hpp"
#include "TestFramework/TestRunnerEndToEndFreeTest.hpp"
#include "TestFramework/TestRunnerPerformanceTest.hpp"
#include "TestFramework/TestRunnerProjectCreation.hpp"
#include "TestFramework/TestRunnerTest.hpp"
#include "TestFramework/TestRunnerTestCreation.hpp"
#include "TextProcessing/IndentFreeTest.hpp"
#include "TextProcessing/PatternsFreeTest.hpp"
#include "TextProcessing/SubstitutionsFreeTest.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
