////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestIncludes.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace testing;

  try
  {
    const timer_resolution r{1};
    const auto root{project_root(argc, argv)};

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       root/"TestAll"/"TestMain.cpp",
                       root/"TestCommon"/"TestIncludes.hpp",
                       repositories{root}
    };

    runner.add_test_family(
      "Test Runner",
      test_runner_false_positive_test{"False Positive Diagnostics"},
      test_runner_test("Functionality Test"),
      test_runner_test_creation{"Test Creation"},
      test_runner_project_creation{"Project Creation"},
      test_runner_end_to_end_test{"End to End Test"}
    );

    runner.add_test_family(
      "Output",
      output_free_test("Free Test")
    );
  
    runner.add_test_family(
      "Core Diagnostics",
      false_positive_diagnostics{"False Positive Diagnostics"},
      false_negative_diagnostics{"False Negative Diagnostics"}
    );
  
    runner.add_test_family(
      "Extended Diagnostics",
      move_only_false_positive_diagnostics{"Move-Only False Positive Diagnostics"},
      orderable_move_only_false_positive_diagnostics{"Orderable Move-Only False Positive Diagnostics"},
      orderable_regular_false_positive_diagnostics{"Orderable Regular False Positive Diagnostics"},
      move_only_false_negative_diagnostics{"Move-Only False Negative Diagnostics"},
      orderable_move_only_false_negative_diagnostics{"Orderable Move-Only False Negative Diagnostics"},
      orderable_regular_false_negative_diagnostics{"Orderable Regular False Negative Diagnostics"}
    );

    runner.add_test_family(
      "Allocation Diagnostics",
      allocation_false_positive_diagnostics{"Alloction False Positive Diagnostics"},
      move_only_allocation_false_positive_diagnostics{"Move-Only Alloction False Positive Diagnostics"},
      allocation_false_negative_diagnostics{"Allocation False Negative Diagnostics"},
      move_only_allocation_false_negative_diagnostics{"Move-Only Allocation False Negative Diagnostics"}
    );

    runner.add_test_family(
      "Scoped Allocation Diagnostics",
      scoped_allocation_false_positive_diagnostics{"Scoped Allocation False Positive Diagnostics"},
      move_only_scoped_allocation_false_positive_diagnostics{"Move-Only Scoped Allocation False Positive Diagnostics"},    
      scoped_allocation_false_negative_diagnostics{"Scoped Allocation False Negative Diagnostics"},
      move_only_scoped_allocation_false_negative_diagnostics{"Move-Only Scoped Allocation False Negative Diagnostics"}
    );

    runner.add_test_family(
      "Performance Diagnostics",
      performance_false_positive_diagnostics{"Performance False Positive Diagnostics"},
      performance_false_negative_diagnostics{"Performance False Negative Diagnostics"},
      performance_utilities_test{"Performance Utilities"}
    );

    runner.add_test_family(
      "Fuzzy Diagnostics",
      fuzzy_false_positive_diagnostics{"Fuzzy False Positive Diagnostics"},
      fuzzy_false_negative_diagnostics{"Fuzzy False Negative Diagnostics"}
    );

    runner.add_test_family(
      "Text Processing",
      indent_free_test("Free Test")
    );

    runner.add_test_family(
      "CommandLine Arguments",
      commandline_arguments_false_positive_test{"False Positive Test"},
      commandline_arguments_test{"Unit Test"}
    );

    runner.add_test_family(
      "Factory",
      factory_false_positive_test("False Positive Test"),
      factory_test("Unit Test")
    );

    runner.add_test_family(
      "Meta",
      type_traits_test{"Type Traits"},
      concepts_test{"Concepts"},
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
      "Linear Sequence",
      linear_sequence_false_positive_test{"False Positive Diagnostics"},
      linear_sequence_test{"Unit Test"}
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
      uniform_wrapper_false_positive_test{"False Positive Diagnostics"},
      uniform_wrapper_test{"Unit Test"}
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
      "StaticLinearlyPartitionedSequence",
      static_linearly_partitioned_sequence_false_positive_test("False Positive Test"),
      static_linearly_partitioned_sequence_test("Unit Test")
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
      test_graph_false_positives{"Graph false positive diagnostics"},
      test_graph_meta("Meta Tests"),
      test_graph_init("Dynamic Graph Init"),
      test_static_graph{"Static Graph Init"},
      test_heterogeneous_static_graph{"Heterogeneous Static Graphs"},
      unweighted_graph_test{"Unweighted Graph Tests"},
      weighted_graph_test{"Weighted Graph Tests"},
      unweighted_graph_allocation_test{"Unweighted Graph Allocation Tests"},
      weighted_graph_allocation_test{"Weighted Graph Allocation Tests"},
      test_fixed_topology{"Dynamic Graph Fixed Topology"},
      test_static_fixed_topology{"Static Graph Manipulations"},
      test_edge_insertion{"Edge Insertions"}
    );

    runner.add_test_family(
      "Graph Algorithms",
      test_graph_traversals{"Traversals"},
      test_static_graph_traversals{"Static Graph Traversals"},
      test_graph_update{"Updates"},
      test_subgraph{"Subgraph"}
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

