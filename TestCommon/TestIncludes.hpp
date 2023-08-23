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
#include "Core/DataStructures/BucketedSequenceRegularTest.hpp"
#include "Core/DataStructures/PartitionedDataAllocationTest.hpp"
#include "Core/DataStructures/PartitionedDataTest.hpp"
#include "Core/DataStructures/PartitionedDataTestingDiagnostics.hpp"
#include "Core/DataStructures/PartitionedSequenceRegularTest.hpp"
#include "Core/DataStructures/StaticLinearlyPartitionedSequenceTest.hpp"
#include "Core/DataStructures/StaticLinearlyPartitionedSequenceTestingDiagnostics.hpp"
#include "Core/DataStructures/StaticPriorityQueueTest.hpp"
#include "Core/DataStructures/StaticPriorityQueueTestingDiagnostics.hpp"
#include "Core/DataStructures/StaticQueueTest.hpp"
#include "Core/DataStructures/StaticQueueTestingDiagnostics.hpp"
#include "Core/DataStructures/StaticStackTest.hpp"
#include "Core/DataStructures/StaticStackTestingDiagnostics.hpp"
#include "Core/Logic/BitmaskFreeTest.hpp"
#include "Core/Meta/ConceptsTest.hpp"
#include "Core/Meta/TypeTraitsTest.hpp"
#include "Core/Meta/UtilitiesTest.hpp"
#include "Core/Object/CreatorFreeTest.hpp"
#include "Core/Object/DataPoolAllocationTest.hpp"
#include "Core/Object/DataPoolTest.hpp"
#include "Core/Object/FaithfulWrapperTest.hpp"
#include "Core/Object/FaithfulWrapperTestingDiagnostics.hpp"
#include "Core/Object/SuiteFreeTest.hpp"
#include "Experimental/ExperimentalTest.hpp"
#include "Experimental/TypeListFreeTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphUnweightedContiguousTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/DynamicGraphFixedTopologyTest.hpp"
#include "Maths/Graph/DynamicGraphTestingDiagnostics.hpp"
#include "Maths/Graph/DynamicGraphTraversalsTest.hpp"
#include "Maths/Graph/DynamicGraphUnweightedAllocationTest.hpp"
#include "Maths/Graph/DynamicGraphUpdateTest.hpp"
#include "Maths/Graph/DynamicGraphWeightedAllocationTest.hpp"
#include "Maths/Graph/DynamicSubgraphTest.hpp"
#include "Maths/Graph/EdgeTest.hpp"
#include "Maths/Graph/EdgeTestingDiagnostics.hpp"
#include "Maths/Graph/GraphMetaTest.hpp"
#include "Maths/Graph/HeterogeneousNodeStorageTest.hpp"
#include "Maths/Graph/HeterogeneousStaticGraphTest.hpp"
#include "Maths/Graph/NodeStorageAllocationTest.hpp"
#include "Maths/Graph/NodeStorageTest.hpp"
#include "Maths/Graph/StaticGraphInitializationTest.hpp"
#include "Maths/Graph/StaticGraphManipulationsTest.hpp"
#include "Maths/Graph/StaticGraphTraversalsTest.hpp"
#include "Maths/Graph/TreeTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphSharedFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphSharedFundamentalWeightTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphSharedUnsortableWeightTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphMetaDataTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphUnsortableWeightTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphUnweightedContiguousTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphSharedFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphSharedFundamentalWeightTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFundamentalWeightContiguousTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFundamentalWeightTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphMetaDataTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedContiguousTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphUnweightedTest.hpp"
#include "Maths/Graph/TreeTestingDiagnostics.hpp"
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
#include "TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnostics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnosticsBrokenSemantics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnosticsBrokenValueSemantics.hpp"
#include "TestFramework/AllocationTesting/RegularAllocationTestFalsePositiveDiagnosticsInefficientOperations.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnostics.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnosticsMixed.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalseNegativeDiagnosticsThreeLevel.hpp"
#include "TestFramework/AllocationTesting/ScopedAllocationTestFalsePositiveDiagnostics.hpp"
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
#include "TestFramework/FunctionFreeDiagnostics.hpp"
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
