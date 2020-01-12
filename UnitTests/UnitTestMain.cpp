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

#include "ExperimentalTest.hpp"

#include "UnitTestDiagnostics.hpp"
#include "UnitTestRunnerTest.hpp"
#include "UnitTestRunner.hpp"

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace unit_testing;

  try
  {
    unit_test_runner runner{argc, argv};
  
    runner.add_test_family(
      test_family{
        "Diagnostics",
        false_positive_diagnostics{"False Positive Diagnostics"},
        false_negative_diagnostics{"False Negative Diagnostics"},
        unit_test_runner_test{"Unit Test Runner"}
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
        test_graph_false_positives{"Graph false positive diagnostics"},
        test_graph_meta("Meta Tests"),
        test_graph_init("Dynamic Graph Init"),        
        test_static_graph{"Static Graph Init"},
        test_heterogeneous_static_graph{"Heterogeneous Static Graphs"},
        test_graph{"Dynmaic Graph Common Tests"},
        test_fixed_topology{"Dynamic Graph Fixed Topology"},
        test_static_fixed_topology{"Static Graph Manipulations"},
        test_edge_insertion{"Edge Insertions"}  
      }
    );

    runner.add_test_family(
      test_family{
        "Graph Algorithms",
        test_graph_traversals{"Traversals"},
        test_static_graph_traversals{"Static Graph Traversals"},
        test_graph_update{"Updates"},
        test_subgraph{"Subgraph"}
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
  catch(const argument_error& e)
  {
    std::cout << e.what();
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

