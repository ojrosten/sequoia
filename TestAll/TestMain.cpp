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
  auto code{sequoia::testing::return_code::incomplete_run};

  try
  {
    using namespace sequoia;
    using namespace object;
    using namespace testing;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestAll/TestMain.cpp"}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.add_test_suite(
      "Test Runner",
      test_runner_false_negative_test{},
      test_runner_test{},
      test_runner_performance_test{},
      test_runner_test_creation{},
      test_runner_project_creation{},
      test_runner_end_to_end_test{},
      test_runner_project_files{}
    );

    runner.add_test_suite(
      "Test Framework Auxiliary",
      versioned_output_free_test{},
      file_editors_free_test{},
      individual_test_paths_free_test{},
      basic_test_interface_free_test{},
      commands_free_test{},
      failure_info_test{},
      failure_info_false_negative_test{},
      file_system_utilities_free_test{},
      output_free_test{},
      dependency_analyzer_free_test{},
      materials_updater_free_test{}
    );

    runner.add_test_suite(
      "Core Diagnostics",
      free_checkers_meta_free_test{},
      elementary_false_negative_free_diagnostics{},
      elementary_false_positive_free_diagnostics{},
      exceptions_false_negative_free_diagnostics{},
      exceptions_false_positive_free_diagnostics{},
      chrono_false_negative_free_diagnostics{},
      chrono_false_positive_free_diagnostics{},
      complex_false_negative_free_diagnostics{},
      complex_false_positive_free_diagnostics{},
      container_false_negative_free_diagnostics{},
      container_false_positive_free_diagnostics{},
      path_false_negative_free_diagnostics{},
      path_false_positive_free_diagnostics{},
      string_false_negative_free_diagnostics{},
      string_false_positive_free_diagnostics{},
      sum_types_false_negative_free_diagnostics{},
      sum_types_false_positive_free_diagnostics{},
      smart_pointer_false_negative_free_diagnostics{},
      smart_pointer_false_positive_free_diagnostics{},
      function_false_negative_free_diagnostics{},
      function_false_positive_free_diagnostics{}
    );
  
    runner.add_test_suite(
      "Semantics Testing Diagnostics",
      regular_false_negative_diagnostics{},
      move_only_false_negative_diagnostics{},
      orderable_move_only_false_negative_diagnostics{},
      orderable_regular_false_negative_diagnostics{},
      regular_false_positive_diagnostics{},
      move_only_false_positive_diagnostics{},
      orderable_move_only_false_positive_diagnostics{},
      orderable_regular_false_positive_diagnostics{}
    );

    runner.add_test_suite(
      "Allocation Diagnostics",
      allocation_false_negative_diagnostics{},
      allocation_false_negative_diagnostics_broken_semantics{},
      allocation_false_negative_diagnostics_broken_value_semantics{},
      allocation_false_negative_diagnostics_inefficient_operations{},
      move_only_allocation_false_negative_diagnostics{},
      allocation_false_positive_diagnostics{},
      move_only_allocation_false_positive_diagnostics{}
    );

    runner.add_test_suite(
      "Scoped Allocation Diagnostics",
      scoped_allocation_false_negative_diagnostics{},
      move_only_scoped_allocation_false_negative_diagnostics{},
      scoped_allocation_false_positive_diagnostics{},
      scoped_allocation_false_positive_diagnostics_mixed{},
      scoped_allocation_false_positive_diagnostics_three_level{},
      move_only_scoped_allocation_false_positive_diagnostics{}
    );

    runner.add_test_suite(
      "Extended Allocation Diagnostics",
      orderable_move_only_allocation_false_positive_diagnostics{},
      orderable_regular_allocation_false_positive_diagnostics{},
      orderable_regular_allocation_false_negative_diagnostics{}
    );

    runner.add_test_suite(
      "Performance Diagnostics",
      performance_false_negative_diagnostics{},
      performance_false_positive_diagnostics{},
      performance_utilities_test{}
    );

    runner.add_test_suite(
      "Relational Diagnostics",
      relational_false_negative_diagnostics{},
      relational_false_positive_diagnostics{}
    );

    runner.add_test_suite(
      "State Transition Utilities",
      regular_state_transition_false_negative_diagnostics{},
      regular_state_transition_false_positive_diagnostics{},
      move_only_state_transition_false_positive_diagnostics{},
      move_only_state_transition_false_negative_diagnostics{}
    );

    runner.add_test_suite(
      "Text Processing",
      indent_free_test{},
      patterns_free_test{},
      substitutions_free_test{}
    );

    runner.add_test_suite(
      "CommandLine Arguments",
      commandline_arguments_false_negative_test{},
      commandline_arguments_test{}
    );

    runner.add_test_suite(
      "Factory",
      factory_false_negative_test{},
      factory_test{}
    );

    runner.add_test_suite(
      "Shell Commands",
      shell_commands_false_negative_test{},
      shell_commands_test{}
    );

    runner.add_test_suite(
      "Meta",
      type_name_free_test{},
      sequences_free_test{},
      type_algorithms_free_test{},
      type_list_free_test{},
      type_traits_test{},
      concepts_test{},
      utilities_test{}
    );

    runner.add_test_suite(
      "Creator",
      creator_free_test{}
    );
    
    runner.add_test_suite(
      "Algorithms",
      algorithms_test{}
    );
  
    runner.add_test_suite(
      "Statistical Algorithms",
      statistical_algorithms_test{}
    );

    runner.add_test_suite(
      "Monotonic Sequence",
      monotonic_sequence_false_negative_test{},
      monotonic_sequence_test{},
      monotonic_sequence_allocation_test{}
    );

    runner.add_test_suite(
      "Linear Sequence",
      linear_sequence_false_negative_test{},
      linear_sequence_test{}
    );

    runner.add_test_suite(
      "Array Utilities",
      array_utilities_test{}
    );
    
    runner.add_test_suite(
        "Iterator",
        iterator_test{}
    );

    runner.add_test_suite(
      "Concurrency Models",
      threading_models_test{},
      threading_models_performance_test{}
    );

    runner.add_test_suite(
      "Partitioned Data",
      suite{
        "Infrastructure",
        partitioned_data_false_negative_test{},
        static_linearly_partitioned_sequence_false_negative_test{},
        partition_iterator_test{},
      },
      suite{
        "Bucketed",
        bucketed_sequence_regular_test{},
        bucketed_sequence_allocation_test{},
      },
      suite{
        "Contiguous",
        partitioned_sequence_regular_test{},
        partitioned_sequence_allocation_test{},
      },
      suite{
        "Static",
        static_partitioned_sequence_test{},
        static_linearly_partitioned_sequence_test{}
      }
    );

    runner.add_test_suite(
      "Static Stack",
      test_static_stack_false_negatives{},
      test_static_stack{}
    );

    runner.add_test_suite(
      "Static Queue",
      test_static_queue_false_negatives{},
      test_static_queue{}
    );

    runner.add_test_suite(
      "Static Priority Queue",
      test_static_priority_queue_false_negatives{},
      test_static_priority_queue{}
    );

    runner.add_test_suite(
      "Graph",
      suite{
        "Infrastructure",
        test_graph_false_negatives{},
        test_graph_meta{}
      },
      suite{
        "Edges",
        test_edge_false_negatives{},
        test_edges{}
      },
      suite{
        "Node Storage",
         node_storage_test{},
         node_storage_allocation_test{},
         test_heterogeneous_node_storage{}
      },
      suite{
        "Dynamic",
        suite{
          "Directed",
          dynamic_directed_graph_unweighted_test{},
          dynamic_directed_graph_unweighted_contiguous_test{},
          dynamic_directed_graph_fundamental_weight_test{},
          dynamic_directed_graph_fundamental_weight_contiguous_test{}
        },
        suite{
          "Undirected",
          dynamic_undirected_graph_unweighted_test{},
          dynamic_undirected_graph_unweighted_contiguous_test{},
          dynamic_undirected_graph_fundamental_weight_test{},
          dynamic_undirected_graph_fundamental_weight_contiguous_test{},
          dynamic_undirected_graph_unsortable_weight_test{},
          dynamic_undirected_graph_shared_fundamental_weight_test{},
          dynamic_undirected_graph_shared_fundamental_weight_contiguous_test{},
          dynamic_undirected_graph_shared_unsortable_weight_test{},
          dynamic_undirected_graph_meta_data_test{}
        },
        suite{
          "Undirected Embedded",
          dynamic_undirected_embedded_graph_unweighted_test{},
          dynamic_undirected_embedded_graph_unweighted_contiguous_test{},
          dynamic_undirected_embedded_graph_fundamental_weight_test{},
          dynamic_undirected_embedded_graph_fundamental_weight_contiguous_test{},
          dynamic_undirected_embedded_graph_shared_fundamental_weight_test{},
          dynamic_undirected_embedded_graph_shared_fundamental_weight_contiguous_test{},
          dynamic_undirected_embedded_graph_meta_data_test{}
        }
      },
      suite{
        "Static",
        suite{
          "Directed",
          static_directed_graph_unweighted_test{},
          static_directed_graph_fundamental_weight_test{}
        },
        suite{
          "Undirected",
          static_undirected_graph_unweighted_test{},
          static_undirected_graph_fundamental_weight_test{},
          static_undirected_graph_unsortable_weight_test{}
        },
        suite{
          "Undirected Embedded",
          static_undirected_embedded_graph_unweighted_test{},
          static_undirected_embedded_graph_fundamental_weight_test{}
        }
      },
      suite{
        "Legacy",
        test_heterogeneous_static_graph{},
      },
      suite{
        "Allocations",
        weighted_graph_allocation_bucketed_test{},
        weighted_graph_allocation_contiguous_test{},
        unweighted_graph_allocation_bucketed_test{},
        unweighted_graph_allocation_contiguous_test{}
      }
    );

    runner.add_test_suite(
      "Graph Algorithms",
      test_graph_traversals{},
      test_static_graph_traversals{},
      test_graph_update{},
      test_subgraph{}
    );

    runner.add_test_suite(
      "Experimental",
      experimental_test{},
      flatten_type_list_free_test{}
    );

    runner.add_test_suite(
      "Streaming",
      streaming_free_test{}
    );

    runner.add_test_suite(
      "Tree",
      tree_false_negative_test{},
      tree_test{}
    );

    runner.add_test_suite(
      "Bitmask",
      bitmask_free_test{}
    );

    runner.add_test_suite(
      "Suite",
      suite_free_test{}
    );
      
    runner.add_test_suite(
      "File System",
      file_system_free_test{},
      normal_path_false_negative_test{},
      normal_path_test{}
    );

    runner.add_test_suite(
      "Algebra",
      suite{
        "Ratio",
        ratio_free_test{}
      }
    );
    
    runner.add_test_suite(
      "Geometry",
      suite{
        "Spaces",        
        numeric_rings_meta_free_test{},
        spaces_meta_free_test{}
      },
      suite{
        "Bounds",
        bounds_free_test{}
      },
      suite{
        "Validators",
        validators_free_test{}
      },
      suite{
        "Vector Coordinates",
        vector_coordinates_false_negative_test{},
        vector_coordinates_test{},
        complex_vector_coordinates_test{},
        vector_polar_coordinates_test{}
      },
      suite{
        "Affine Coordinates",
        affine_coordinates_false_negative_test{},
        affine_coordinates_test{}
      },
      suite{
        "M-Affine Coordinates",
        m_affine_coordinates_test{}
      },
      suite{
        "Free Module Coordinates",
        free_module_coordinates_test{}
      },
      suite{
        "Partial M-Torsor Coordinates",
        partial_m_torsor_coordinates_test{}
      },
      suite{
        "Absolute Coordinates",
        absolute_coordinates_false_negative_test{},
        absolute_coordinates_test{},
        absolute_logarithmic_coordinates_test{}
      }
    );

    runner.add_test_suite(
      "Physical Values",
      space_ordering_meta_free_test{},
      physical_value_meta_free_test{},
      physical_value_false_negative_test{},
      physical_value_conversions_free_test{},
      absolute_physical_value_test{},
      absolute_physical_value_compositions_test{},
      unsafe_absolute_physical_value_test{},
      unsafe_absolute_physical_value_compositions_test{},
      affine_physical_value_test{},
      convex_physical_value_test{},
      vector_physical_value_test{},
      vector_physical_value_compositions_test{},
      mixed_physical_value_test{},
      integral_physical_value_test{}
    );

    runner.add_test_suite(
      "Mem Ordered Tuple",
      mem_ordered_tuple_false_negative_test{},
      mem_ordered_tuple_test{}
    );

    runner.add_test_suite(
      "Saturating Arithmetic",
      saturating_mul_free_test{},
      saturating_add_free_test{}
    );

    runner.add_test_suite(
      "Vector_nonlinear_representations",
      vector_nonlinear_representations_free_test{}
    );

    code = runner.execute(timer_resolution{1ms});
  }
  catch(const std::exception& e)
  {
    std::cout << e.what();
  }
  catch(...)
  {
    std::cout << "Unrecognized error\n";
  }

  return sequoia::testing::to_exit_code(code);
}

