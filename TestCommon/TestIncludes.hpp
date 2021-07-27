////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Algorithms/AlgorithmsTest.hpp"
#include "Core/Concurrency/ConcurrencyModelsPerformanceTest.hpp"
#include "Core/Concurrency/ConcurrencyModelsTest.hpp"
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
#include "Core/Meta/ConceptsTest.hpp"
#include "Core/Meta/TypeTraitsTest.hpp"
#include "Core/Meta/UtilitiesTest.hpp"
#include "Core/Ownership/DataPoolAllocationTest.hpp"
#include "Core/Ownership/DataPoolTest.hpp"
#include "Core/Utilities/ArrayUtilitiesTest.hpp"
#include "Core/Utilities/IteratorTest.hpp"
#include "Core/Utilities/UniformWrapperTest.hpp"
#include "Core/Utilities/UniformWrapperTestingDiagnostics.hpp"
#include "Experimental/ExperimentalTest.hpp"
#include "Maths/Graph/DynamicGraphEmbeddedTest.hpp"
#include "Maths/Graph/DynamicGraphFixedTopologyTest.hpp"
#include "Maths/Graph/DynamicGraphInitializationTest.hpp"
#include "Maths/Graph/DynamicGraphTestingDiagnostics.hpp"
#include "Maths/Graph/DynamicGraphTraversalsTest.hpp"
#include "Maths/Graph/DynamicGraphUnweightedAllocationTest.hpp"
#include "Maths/Graph/DynamicGraphUnweightedTest.hpp"
#include "Maths/Graph/DynamicGraphUpdateTest.hpp"
#include "Maths/Graph/DynamicGraphWeightedAllocationTest.hpp"
#include "Maths/Graph/DynamicGraphWeightedTest.hpp"
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
#include "Streaming/StreamingFreeTest.hpp"
#include "TestFramework/CoreDiagnostics.hpp"
#include "TestFramework/DependencyAnalyzerFreeTest.hpp"
#include "TestFramework/FileSystemFreeTest.hpp"
#include "TestFramework/MoveOnlyAllocationTestDiagnostics.hpp"
#include "TestFramework/MoveOnlyScopedAllocationTestDiagnostics.hpp"
#include "TestFramework/MoveOnlyTestDiagnostics.hpp"
#include "TestFramework/OrderableMoveOnlyAllocationTestDiagnostics.hpp"
#include "TestFramework/OrderableMoveOnlyTestDiagnostics.hpp"
#include "TestFramework/OrderableRegularAllocationTestDiagnostics.hpp"
#include "TestFramework/OrderableRegularTestDiagnostics.hpp"
#include "TestFramework/OutputFreeTest.hpp"
#include "TestFramework/PerformanceTestDiagnostics.hpp"
#include "TestFramework/RegularAllocationTestDiagnostics.hpp"
#include "TestFramework/RelationalTestDiagnostics.hpp"
#include "TestFramework/ScopedAllocationTestDiagnostics.hpp"
#include "TestFramework/TestRunnerDiagnostics.hpp"
#include "TestFramework/TestRunnerEndToEndFreeTest.hpp"
#include "TestFramework/TestRunnerProjectCreation.hpp"
#include "TestFramework/TestRunnerTest.hpp"
#include "TestFramework/TestRunnerTestCreation.hpp"
#include "TextProcessing/IndentFreeTest.hpp"
#include "TextProcessing/PatternsFreeTest.hpp"
#include "TextProcessing/SubstitutionsFreeTest.hpp"
#include "sequoia/TestFramework/TestRunner.hpp"
