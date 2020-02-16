////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "AlgorithmsTest.hpp"
#include "StatisticalAlgorithmsTest.hpp"
#include "MonotonicSequenceTestingDiagnostics.hpp"
#include "MonotonicSequenceTest.hpp"
#include "MonotonicSequenceAllocationTest.hpp"

#include "TypeTraitsTest.hpp"
#include "UtilitiesTest.hpp"

#include "ArrayUtilitiesTest.hpp"
#include "ProtectiveWrapperTestingDiagnostics.hpp"
#include "ProtectiveWrapperTest.hpp"
#include "IteratorTest.hpp"

#include "DataPoolTest.hpp"
#include "DataPoolAllocationTest.hpp"

#include "ConcurrencyModelsTest.hpp"
#include "ConcurrencyModelsPerformanceTest.hpp"

#include "PartitionedDataTest.hpp"
#include "PartitionedDataAllocationTest.hpp"
#include "PartitionedDataTestingDiagnostics.hpp"

#include "StaticStackTest.hpp"
#include "StaticQueueTest.hpp"
#include "StaticPriorityQueueTest.hpp"
#include "StaticStackTestingDiagnostics.hpp"
#include "StaticQueueTestingDiagnostics.hpp"
#include "StaticPriorityQueueTestingDiagnostics.hpp"

#include "EdgeTest.hpp"
#include "EdgeTestingDiagnostics.hpp"

#include "NodeStorageTest.hpp"
#include "NodeStorageAllocationTest.hpp"
#include "HeterogeneousNodeStorageTest.hpp"

#include "GraphMetaTest.hpp"

#include "DynamicGraphTestingDiagnostics.hpp"
#include "DynamicGraphInitializationTest.hpp"
#include "DynamicGraphUnweightedTest.hpp"
#include "DynamicGraphWeightedTest.hpp"
#include "DynamicGraphUnweightedAllocationTest.hpp"
#include "DynamicGraphWeightedAllocationTest.hpp"
#include "DynamicGraphEmbeddedTest.hpp"
#include "DynamicGraphFixedTopologyTest.hpp"

#include "StaticGraphInitializationTest.hpp"
#include "StaticGraphManipulationsTest.hpp"

#include "HeterogeneousStaticGraphTest.hpp"

#include "DynamicGraphTraversalsTest.hpp"
#include "DynamicGraphUpdateTest.hpp"
#include "DynamicSubgraphTest.hpp"

#include "StaticGraphTraversalsTest.hpp"

#include "ExperimentalTest.hpp"

#include "CommandLineArgumentsDiagnostics.hpp"
#include "CommandLineArgumentsTest.hpp"

#include "UnitTestDiagnostics.hpp"
#include "MoveOnlyTestDiagnostics.hpp"
#include "PerformanceTestDiagnostics.hpp"
#include "AllocationTestDiagnostics.hpp"
#include "MoveOnlyAllocationTestDiagnostics.hpp"

#include "UnitTestRunner.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace unit_testing;

  try
  {
    unit_test_runner runner{argc, argv};
    const auto mode{runner.concurrency()};
  
    runner.add_test_family(
      "Diagnostics",
      false_positive_diagnostics{"False Positive Diagnostics"},
      move_only_false_positive_diagnostics{"Move-Only False Positive Diagnostics"},
      performance_false_positive_diagnostics{"Performance False Positive Diagnostics"},
      allocation_false_positive_diagnostics{"Alloction False Positive Diagnostics"},
      move_only_allocation_false_positive_diagnostics{"Move-Only Alloction False Positive Diagnostics"},
      false_negative_diagnostics{"False Negative Diagnostics"},
      move_only_false_negative_diagnostics{"Move-Only False Negative Diagnostics"},
      performance_false_negative_diagnostics{"Performance False Negative Diagnostics"},
      allocation_false_negative_diagnostics{"Allocation False Negative Diagnostics"},
      move_only_allocation_false_negative_diagnostics{"Move-Only Allocation False Negative Diagnostics"}
    );

    runner.add_test_family(
      "CommandLine Arguments",
      commandline_arguments_test{"Unit Test"},
      commandline_arguments_false_positive_test{"False Positive Test"}
    );

    runner.add_test_family(
      "Meta",
      type_traits_test{"Type Traits"},
      utilities_test{"Utilities"}
    );
    
    runner.add_test_family(
      "Algorithms",
      algorithms_test{"Unit Test"}
    );
  
    runner.add_test_family(
      "Statistical Algorithms",
      statistical_algorithms_test{"Unit Test"}
    );

    runner.add_test_family(
      "Monotonic Sequence",
      monotonic_sequence_false_positive_test{"False Positive Diagnostics"},
      monotonic_sequence_test{"Unit Test"},
      monotonic_sequence_allocation_test{"Allocation Test"}
    );

    runner.add_test_family(
      "Array Utilities",
      array_utilities_test{"Unit Test"}
    );
    
    runner.add_test_family(
        "Iterator",
        iterator_test{"Unit Test"}
    );

    runner.add_test_family(
      "Protective Wrapper",
      protective_wrapper_false_positive_test{"False Positive Diagnostics"},
      protective_wrapper_test{"Unit Test"}
    );    

    runner.add_test_family(
      "Data Pool",
      data_pool_test{"Unit Test"},
      data_pool_allocation_test{"Allocation Test"}
    );

    runner.add_test_family(
      "Concurrency Models",
      threading_models_test{"Unit Test"},
      threading_models_performance_test{"Performance Test"}
    );

    runner.add_test_family(
      "Partitioned Data",
      partitioned_data_false_positive_test{"False Positive Diagnostics"},
      partitioned_data_test{"Unit Test"},
      partitioned_data_allocation_test{"Allocation Test"}
    );

    runner.add_test_family(
      "Static Stack",
      test_static_stack_false_positives{"False Positive Diagnostics"},
      test_static_stack{"Unit Test"}
    );

    runner.add_test_family(
      "Static Queue",
      test_static_queue_false_positives{"False Positive Diagnostics"},
      test_static_queue{"Unit Test"}
    );

    runner.add_test_family(
      "Static Priority Queue",
      test_static_priority_queue_false_positives{"False Positive Diagnostics"},
      test_static_priority_queue{"Unit Test"}
    );
  
    runner.add_test_family(
      "Edges",
      test_edge_false_positives{"False positive diagnostics"},
      test_edges{"Unit Test"}
    );

    runner.add_test_family(
      "Node Storage",
      node_storage_test{"Dynamic and Static"},
      node_storage_allocation_test{"Allocation Test"},
      test_heterogeneous_node_storage{"Heterogeneuous"}
    );

    runner.add_test_family(
      "Graph",
      test_graph_false_positives{"Graph false positive diagnostics", mode},
      test_graph_meta("Meta Tests", mode),
      test_graph_init("Dynamic Graph Init", mode),        
      test_static_graph{"Static Graph Init", mode},
      test_heterogeneous_static_graph{"Heterogeneous Static Graphs", mode},
      unweighted_graph_test{"Unweighted Graph Tests", mode},
      weighted_graph_test{"Weighted Graph Tests", mode},
      unweighted_graph_allocation_test{"Unweighted Graph Allocation Tests", mode},
      weighted_graph_allocation_test{"Weighted Graph Allocation Tests", mode},
      test_fixed_topology{"Dynamic Graph Fixed Topology", mode},
      test_static_fixed_topology{"Static Graph Manipulations", mode},
      test_edge_insertion{"Edge Insertions", mode}  
    );

    runner.add_test_family(
      "Graph Algorithms",
      test_graph_traversals{"Traversals", mode},
      test_static_graph_traversals{"Static Graph Traversals", mode},
      test_graph_update{"Updates", mode},
      test_subgraph{"Subgraph", mode}
    );

    runner.add_test_family(
      "Experimental",
      experimental_test{"Unit Test"}
    );
 
    runner.execute();
  }
  catch(const std::exception& e)
  {
    std::cout << e.what();
  }
  catch(...)
  {
    std::cout << "Unrecognized error\n"; 
  }
  
  return 0;
}

