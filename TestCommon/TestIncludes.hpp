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
#include "Core/DataStructures/PartitionedDataAllocationTest.hpp"
#include "Core/DataStructures/PartitionedDataTest.hpp"
#include "Core/DataStructures/PartitionedDataTestingDiagnostics.hpp"
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
#include "Maths/Graph/Directed/DynamicDirectedGraphFaithfulFaithfulTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphFaithfulPoolTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphPoolFaithfulTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphPoolPoolTest.hpp"
#include "Maths/Graph/Directed/DynamicDirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphFaithfulFaithfulSharedEdgeTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphFaithfulFaithfulSharedWeightTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphFaithfulFaithfulTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphFaithfulPoolTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphPoolFaithfulSharedEdgeTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphPoolFaithfulSharedWeightTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphPoolFaithfulTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphPoolPoolTest.hpp"
#include "Maths/Graph/DirectedEmbedded/DynamicDirectedEmbeddedGraphUnweightedTest.hpp"
#include "Maths/Graph/DynamicGraphFixedTopologyTest.hpp"
#include "Maths/Graph/DynamicGraphInitializationTest.hpp"
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
#include "Maths/Graph/Undirected/DynamicUndirectedGraphFaithfulFaithfulSharedWeightTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphFaithfulFaithfulTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphPoolFaithfulSharedWeightTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphFaithfulPoolTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphPoolFaithfulTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphPoolPoolTest.hpp"
#include "Maths/Graph/Undirected/DynamicUndirectedGraphUnweightedTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFaithfulFaithfulSharedWeightTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFaithfulFaithfulTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphFaithfulPoolTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphPoolFaithfulSharedWeightTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphPoolFaithfulTest.hpp"
#include "Maths/Graph/UndirectedEmbedded/DynamicUndirectedEmbeddedGraphPoolPoolTest.hpp"
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
#include "TestFramework/MoveOnlyAllocationTestDiagnostics.hpp"
#include "TestFramework/MoveOnlyScopedAllocationTestDiagnostics.hpp"
#include "TestFramework/MoveOnlyStateTransitionDiagnostics.hpp"
#include "TestFramework/MoveOnlyTestDiagnostics.hpp"
#include "TestFramework/OrderableMoveOnlyAllocationTestDiagnostics.hpp"
#include "TestFramework/OrderableMoveOnlyTestDiagnostics.hpp"
#include "TestFramework/OrderableRegularAllocationTestDiagnostics.hpp"
#include "TestFramework/OrderableRegularTestDiagnostics.hpp"
#include "TestFramework/OutputFreeTest.hpp"
#include "TestFramework/PathFreeDiagnostics.hpp"
#include "TestFramework/PerformanceTestDiagnostics.hpp"
#include "TestFramework/RegularAllocationTestFalseNegativeDiagnostics.hpp"
#include "TestFramework/RegularAllocationTestFalsePositiveDiagnostics.hpp"
#include "TestFramework/RegularAllocationTestFalsePositiveDiagnosticsBrokenSemantics.hpp"
#include "TestFramework/RegularAllocationTestFalsePositiveDiagnosticsBrokenValueSemantics.hpp"
#include "TestFramework/RegularAllocationTestFalsePositiveDiagnosticsInefficientOperations.hpp"
#include "TestFramework/RegularStateTransitionDiagnostics.hpp"
#include "TestFramework/RegularTestDiagnostics.hpp"
#include "TestFramework/RelationalTestDiagnostics.hpp"
#include "TestFramework/ScopedAllocationTestFalseNegativeDiagnostics.hpp"
#include "TestFramework/ScopedAllocationTestFalseNegativeDiagnosticsMixed.hpp"
#include "TestFramework/ScopedAllocationTestFalsePositiveDiagnostics.hpp"
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
