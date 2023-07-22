////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestIncludes.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  try
  {
    using namespace sequoia;
    using namespace object;
    using namespace testing;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       {"TestAll/TestMain.cpp", {}, "TestCommon/TestIncludes.hpp"}};

    runner.add_test_suite(
      "Test Runner",
      test_runner_false_positive_test{"False Positive Diagnostics"},
      test_runner_test{"Functionality Test"},
      test_runner_performance_test{"Test Runner Performance Test"},
      test_runner_test_creation{"Test Creation"},
      test_runner_project_creation{"Project Creation"},
      test_runner_end_to_end_test{"End to End Test"}
    );

    runner.add_test_suite(
      "Test Framework Auxiliary",
      commands_free_test{"Commands Free Test"},
      failure_info_test{"failure_info Unit Test"},
      failure_info_false_positive_test{"failure_info False Positive Test"},
      file_system_utilities_free_test{"File System Free Test"},
      output_free_test{"Output Free Test"},
      dependency_analyzer_free_test{"Dependency Analyzer Free Test"},
      materials_updater_free_test{"Free Test"}
    );

    runner.add_test_suite(
      "Core Diagnostics",
      elementary_false_positive_free_diagnostics{"Elementary False Positive Free Diagnostics"},
      elementary_false_negative_free_diagnostics{"Elementary False Negative Free Diagnostics"},
      exceptions_false_positive_free_diagnostics{"Exceptions False Positive Free Diagnostics"},
      exceptions_false_negative_free_diagnostics{"Exceptions False Negative Free Diagnostics"},
      chrono_false_positive_free_diagnostics{"Chrono False Positive Free Diagnostics"},
      chrono_false_negative_free_diagnostics{"Chrono False Negative Free Diagnostics"},
      complex_false_positive_free_diagnostics{"Complex False Positive Free Diagnostics"},
      complex_false_negative_free_diagnostics{"Complex False Negative Free Diagnostics"},
      container_false_positive_free_diagnostics{"Container False Positive Free Diagnostics"},
      container_false_negative_free_diagnostics{"Container False Negative Free Diagnostics"},
      path_false_positive_free_diagnostics{"Path False Positive Free Diagnostics"},
      path_false_negative_free_diagnostics{"Path False Negative Free Diagnostics"},
      string_false_positive_free_diagnostics{"String False Positive Free Diagnostics"},
      string_false_negative_free_diagnostics{"String False Negative Free Diagnostics"},
      sum_types_false_positive_free_diagnostics{"Sum Types False Positive Free Diagnostics"},
      sum_types_false_negative_free_diagnostics{"Sum Types False Negative Free Diagnostics"},
      smart_pointer_false_positive_free_diagnostics{"Smart Pointer False Positive Free Diagnostics"},
      smart_pointer_false_negative_free_diagnostics{"Smart Pointer False Negative Free Diagnostics"},
      function_false_positive_free_diagnostics{"Function False Positive Free Diagnostics"},
      function_false_negative_free_diagnostics{"Function False Negative Free Diagnostics"}
    );
  
    runner.add_test_suite(
      "Semantics Testing Diagnostics",
      regular_false_positive_diagnostics{"Regular False Positive Diagnostics"},
      move_only_false_positive_diagnostics{"Move-Only False Positive Diagnostics"},
      orderable_move_only_false_positive_diagnostics{"Orderable Move-Only False Positive Diagnostics"},
      orderable_regular_false_positive_diagnostics{"Orderable Regular False Positive Diagnostics"},
      regular_false_negative_diagnostics{"Regular False Negative Diagnostics"},
      move_only_false_negative_diagnostics{"Move-Only False Negative Diagnostics"},
      orderable_move_only_false_negative_diagnostics{"Orderable Move-Only False Negative Diagnostics"},
      orderable_regular_false_negative_diagnostics{"Orderable Regular False Negative Diagnostics"}
    );

    runner.add_test_suite(
      "Allocation Diagnostics",
      allocation_false_positive_diagnostics{"Allocation False Positive Diagnostics"},
      move_only_allocation_false_positive_diagnostics{"Move-Only Alloction False Positive Diagnostics"},
      allocation_false_negative_diagnostics{"Allocation False Negative Diagnostics"},
      move_only_allocation_false_negative_diagnostics{"Move-Only Allocation False Negative Diagnostics"}
    );

    runner.add_test_suite(
      "Scoped Allocation Diagnostics",
      scoped_allocation_false_positive_diagnostics{"Scoped Allocation False Positive Diagnostics"},
      move_only_scoped_allocation_false_positive_diagnostics{"Move-Only Scoped Allocation False Positive Diagnostics"},
      scoped_allocation_false_negative_diagnostics{"Scoped Allocation False Negative Diagnostics"},
      move_only_scoped_allocation_false_negative_diagnostics{"Move-Only Scoped Allocation False Negative Diagnostics"}
    );

    runner.add_test_suite(
      "Extended Allocation Diagnostics",
      orderable_move_only_allocation_false_negative_diagnostics{"Orderable Move-Only Allocation False Negative Diagnostics"},
      orderable_regular_allocation_false_negative_diagnostics{"Orderable Regular Allocation False Negative Diagnostics"},
      orderable_regular_allocation_false_positive_diagnostics{"Orderable Regular Allocation False Positive Diagnostics"}
    );

    runner.add_test_suite(
      "Performance Diagnostics",
      performance_false_positive_diagnostics{"Performance False Positive Diagnostics"},
      performance_false_negative_diagnostics{"Performance False Negative Diagnostics"},
      performance_utilities_test{"Performance Utilities"}
    );

    runner.add_test_suite(
      "Relational Diagnostics",
      relational_false_positive_diagnostics{"Relational False Positive Diagnostics"},
      relational_false_negative_diagnostics{"Relational False Negative Diagnostics"}
    );

    runner.add_test_suite(
      "State Transition Utilities",
      regular_state_transition_false_positive_diagnostics{"Regular False Positive Diagnostics"},
      regular_state_transition_false_negative_diagnostics{"Regular False Negative Diagnostics"},
      move_only_state_transition_false_negative_diagnostics{"Move-Only False Positive Diagnostics"},
      move_only_state_transition_false_positive_diagnostics{"Move-Only False Negative Diagnostics"}
    );

    runner.add_test_suite(
      "Text Processing",
      indent_free_test{"Indent Free Test"},
      patterns_free_test{"Patterns Free Test"},
      substitutions_free_test{"Substitutions Free Test"}
    );

    runner.add_test_suite(
      "CommandLine Arguments",
      commandline_arguments_false_positive_test{"False Positive Test"},
      commandline_arguments_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Factory",
      factory_false_positive_test{"False Positive Test"},
      factory_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Shell Commands",
      shell_commands_false_positive_test{"False Positive Test"},
      shell_commands_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Meta",
      type_traits_test{"Type Traits"},
      concepts_test{"Concepts"},
      utilities_test{"Utilities"}
    );

    runner.add_test_suite(
      "Creator",
      creator_free_test{"Free Test"}
    );
    
    runner.add_test_suite(
      "Algorithms",
      algorithms_test{"Unit Test"}
    );
  
    runner.add_test_suite(
      "Statistical Algorithms",
      statistical_algorithms_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Monotonic Sequence",
      monotonic_sequence_false_positive_test{"False Positive Diagnostics"},
      monotonic_sequence_test{"Unit Test"},
      monotonic_sequence_allocation_test{"Allocation Test"}
    );

    runner.add_test_suite(
      "Linear Sequence",
      linear_sequence_false_positive_test{"False Positive Diagnostics"},
      linear_sequence_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Array Utilities",
      array_utilities_test{"Unit Test"}
    );
    
    runner.add_test_suite(
        "Iterator",
        iterator_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Faithful Wrapper",
      faithful_wrapper_false_positive_test{"False Positive Diagnostics"},
      faithful_wrapper_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Data Pool",
      data_pool_test{"Unit Test"},
      data_pool_allocation_test{"Allocation Test"}
    );

    runner.add_test_suite(
      "Concurrency Models",
      threading_models_test{"Unit Test"},
      threading_models_performance_test{"Performance Test"}
    );

    runner.add_test_suite(
      "Partitioned Data",
      partitioned_data_false_positive_test{"False Positive Diagnostics"},
      partitioned_data_test{"Unit Test"},
      partitioned_data_allocation_test{"Allocation Test"}
    );

    runner.add_test_suite(
      "StaticLinearlyPartitionedSequence",
      static_linearly_partitioned_sequence_false_positive_test{"False Positive Test"},
      static_linearly_partitioned_sequence_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Static Stack",
      test_static_stack_false_positives{"False Positive Diagnostics"},
      test_static_stack{"Unit Test"}
    );

    runner.add_test_suite(
      "Static Queue",
      test_static_queue_false_positives{"False Positive Diagnostics"},
      test_static_queue{"Unit Test"}
    );

    runner.add_test_suite(
      "Static Priority Queue",
      test_static_priority_queue_false_positives{"False Positive Diagnostics"},
      test_static_priority_queue{"Unit Test"}
    );
  
    runner.add_test_suite(
      "Edges",
      test_edge_false_positives{"False positive diagnostics"},
      test_edges{"Unit Test"}
    );

    runner.add_test_suite(
      "Node Storage",
      node_storage_test{"Dynamic and Static"},
      node_storage_allocation_test{"Allocation Test"},
      test_heterogeneous_node_storage{"Heterogeneuous"}
    );

    runner.add_test_suite(
      "Graph",
      suite{
        "Infrastructure",
        test_graph_false_positives{"Graph false positive diagnostics"},
        test_graph_meta{"Meta Tests"}
      },
      suite{
        "Manipulations",
        suite{
          "Directed",
          dynamic_directed_graph_unweighted_test{"Directed Graph Unweighted Tests"},
          dynamic_directed_graph_faithful_faithful_test{"Directed Graph Faithful-Faithful Tests"},
          dynamic_directed_graph_faithful_pool_test{"Directed Graph Faithful-Pool Tests"},
          dynamic_directed_graph_pool_faithful_test{"Directed Graph Pool-Faithful Tests"},
          dynamic_directed_graph_pool_pool_test{"Directed Graph Pool-Pool Tests"}
        },
        suite{
          "Directed Embedded",
          dynamic_directed_embedded_graph_unweighted_test{"Directed Embedded Graph Unweighted Tests"},
          dynamic_directed_embedded_graph_faithful_faithful_test{"Directed Embedded Graph Faithful-Faithful Tests"},
          dynamic_directed_embedded_graph_faithful_pool_test{"Directed Embedded Graph Faithful-Pool Tests"},
          dynamic_directed_embedded_graph_pool_faithful_test{"Directed Embedded Graph Pool-Faithful Tests"},
          dynamic_directed_embedded_graph_pool_pool_test{"Directed Embedded Graph Pool-Pool Tests"}
        },
        suite{
          "Undirected",
          dynamic_undirected_graph_unweighted_test{"Undirected Graph Unweighted Tests"},
          dynamic_undirected_graph_faithful_faithful_test{"Undirected Graph Faithful-Faithful Tests"},
          dynamic_undirected_graph_faithful_pool_test{"Undirected Graph Faithful-Pool Tests"},
          dynamic_undirected_graph_pool_faithful_test{"Undirected Graph Pool-Faithful Tests"},
          dynamic_undirected_graph_pool_pool_test{"Undirected Graph Pool-Pool Tests"},
          dynamic_undirected_graph_faithful_faithful_shared_weight_test{"Undirected Graph Faithful-Faithful Shared Weight Tests"},
          dynamic_undirected_graph_pool_faithful_shared_weight_test{"Undirected Graph Pool-Faithful Shared Weight Tests"}
        },
        suite{
          "Undirected Embedded",
          dynamic_undirected_embedded_graph_unweighted_test{"Undirected Embedded Graph Unweighted Tests"},
          dynamic_undirected_embedded_graph_faithful_faithful_test{"Undirected Embedded Graph Faithful-Faithful Tests"},
          dynamic_undirected_embedded_graph_faithful_pool_test{"Undirected Embedded Graph Faithful-Pool Tests"},
          dynamic_undirected_embedded_graph_pool_faithful_test{"Undirected Embedded Graph Pool-Faithful Tests"},
          dynamic_undirected_embedded_graph_pool_pool_test{"Undirected Embedded Graph Pool-Pool Tests"}
        },
        suite{
          "Legacy",
          weighted_graph_test{"Weighted Graph Tests"},
          test_static_fixed_topology{"Static Graph Manipulations"},
          test_edge_insertion{"Edge Insertions"},
          test_graph_init{"Dynamic Graph Init"},
          test_static_graph{"Static Graph Init"},
          test_heterogeneous_static_graph{"Heterogeneous Static Graphs"},
          test_fixed_topology{"Dynamic Graph Fixed Topology"}
        }
      },
      suite{
        "Allocations",
        weighted_graph_allocation_test{"Weighted Graph Allocation Tests"},
        unweighted_graph_allocation_test{"Unweighted Graph Allocation Tests"},
      }
    );

    runner.add_test_suite(
      "Graph Algorithms",
      test_graph_traversals{"Traversals"},
      test_static_graph_traversals{"Static Graph Traversals"},
      test_graph_update{"Updates"},
      test_subgraph{"Subgraph"}
    );

    runner.add_test_suite(
      "Experimental",
      experimental_test{"Unit Test"},
      type_list_free_test{"Free Test"}
    );

    runner.add_test_suite(
      "Streaming",
      streaming_free_test{"Free Test"}
    );

    runner.add_test_suite(
      "Tree",
      tree_false_positive_test{"False Positive Test"},
      tree_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Bitmask",
      bitmask_free_test{"Bitmask Free Test"}
    );

    runner.add_test_suite(
      "Suite",
      suite_free_test{"Suite Free Test"}
    );

    runner.execute(timer_resolution{1ms});
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

