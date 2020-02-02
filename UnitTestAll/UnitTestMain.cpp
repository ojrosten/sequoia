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

#include "TypeTraitsTest.hpp"
#include "UtilitiesTest.hpp"

#include "ArrayUtilitiesTest.hpp"
#include "ProtectiveWrapperTestingDiagnostics.hpp"
#include "ProtectiveWrapperTest.hpp"
#include "IteratorTest.hpp"

#include "DataPoolTest.hpp"

#include "ConcurrencyModelsTest.hpp"

#include "PartitionedDataTest.hpp"
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
#include "HeterogeneousNodeStorageTest.hpp"

#include "GraphMetaTest.hpp"

#include "DynamicGraphTestingDiagnostics.hpp"
#include "DynamicGraphInitializationTest.hpp"
#include "DynamicGraphCommonTest.hpp"
#include "DynamicGraphEmbeddedTest.hpp"
#include "DynamicGraphFixedTopologyTest.hpp"

#include "StaticGraphInitializationTest.hpp"
#include "StaticGraphManipulationsTest.hpp"

#include "HeterogeneousStaticGraphTest.hpp"

#include "DynamicGraphTraversalsTest.hpp"
#include "DynamicGraphUpdateTest.hpp"
#include "DynamicSubgraphTest.hpp"

#include "StaticGraphTraversalsTest.hpp"

#include "CommandLineArgumentsDiagnostics.hpp"
#include "CommandLineArgumentsTest.hpp"

#include "ExperimentalTest.hpp"

#include "UnitTestDiagnostics.hpp"
#include "UnitTestAllocatorDiagnostics.hpp"

#include "UnitTestRunner.hpp"

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace unit_testing;

  try
  {
    unit_test_runner runner{argc, argv};
    //const concurrency_flavour asynchronous{runner.asynchronous() ? concurrency_flavour::async : concurrency_flavour::serial};
    const concurrency_flavour asynchronous{concurrency_flavour::serial};
  
    runner.add_test_family(
      test_family{
        "Diagnostics",
        false_positive_diagnostics{"False Positive Diagnostics"},
        allocator_false_positive_diagnostics{"False Positive Diagnostics"},
        false_negative_diagnostics{"Allocator False Negative Diagnostics"},
        allocator_false_negative_diagnostics{"Allocator False Negative Diagnostics"}
      }
    );

    runner.add_test_family(
      test_family{
        "CommandLine Arguments",
        commandline_arguments_test{"Unit Test"},
        commandline_arguments_false_positive_test{"False Positive Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Meta",
        type_traits_test{"Type Traits"},
        utilities_test{"Utilities"}
      }
    );
    
    runner.add_test_family(
      test_family{
        "Algorithms",
        algorithms_test{"Unit Test"}
      }
    );
  
    runner.add_test_family(
      test_family{
        "Statistical Algorithms",
        statistical_algorithms_test{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Monotonic Sequence",
        monotonic_sequence_false_positive_test{"False Positive Diagnostics"},
        monotonic_sequence_test{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Array Utilities",
        array_utilities_test{"Unit Test"}
      }
    );
    
    runner.add_test_family(
      test_family{
        "Iterator",
        iterator_test{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Protective Wrapper",
        protective_wrapper_false_positive_test{"False Positive Diagnostics"},
        protective_wrapper_test{"Unit Test"}
      }
    );    

    runner.add_test_family(
      test_family{
        "Data Pool",
        data_pool_test{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Concurrency Models",
        threading_models_test{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Partitioned Data",
        partitioned_data_false_positive_test{"False Positive Diagnostics"},
        partitioned_data_test{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Static Stack",
        test_static_stack_false_positives{"False Positive Diagnostics"},
        test_static_stack{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Static Queue",
        test_static_queue_false_positives{"False Positive Diagnostics"},
        test_static_queue{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Static Priority Queue",
        test_static_priority_queue_false_positives{"False Positive Diagnostics"},
        test_static_priority_queue{"Unit Test"}
      }
    );
  
    runner.add_test_family(
      test_family{
        "Edges",
        test_edge_false_positives{"False positive diagnostics"},
        test_edges{"Unit Test"}
      }
    );

    runner.add_test_family(
      test_family{
        "Node Storage",
        test_node_storage{"Dynamic and Static"},
        test_heterogeneous_node_storage{"Heterogeneuous"}
      }
    );

    runner.add_test_family(
      test_family{
        "Graph",
        test_graph_false_positives{"Graph false positive diagnostics", asynchronous},
        test_graph_meta("Meta Tests", asynchronous),
        test_graph_init("Dynamic Graph Init", asynchronous),        
        test_static_graph{"Static Graph Init", asynchronous},
        test_heterogeneous_static_graph{"Heterogeneous Static Graphs", asynchronous},
        test_graph{"Dynmaic Graph Common Tests", asynchronous},
        test_fixed_topology{"Dynamic Graph Fixed Topology", asynchronous},
        test_static_fixed_topology{"Static Graph Manipulations", asynchronous},
        test_edge_insertion{"Edge Insertions", asynchronous}  
      }
    );

    runner.add_test_family(
      test_family{
        "Graph Algorithms",
        test_graph_traversals{"Traversals", asynchronous},
        test_static_graph_traversals{"Static Graph Traversals", asynchronous},
        test_graph_update{"Updates", asynchronous},
        test_subgraph{"Subgraph", asynchronous}
      }
    );

    runner.add_test_family(
      test_family{
        "Experimental",
        experimental_test{"Unit Test"}
      }
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

