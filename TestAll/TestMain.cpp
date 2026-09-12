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
                       {.source_folder{"sequoia"}, .main_cpp{"TestAll/TestMain.cpp"}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.register_test<test_runner_false_negative_test>();
    runner.register_test<test_runner_test>();
    runner.register_test<test_runner_performance_test>();
    runner.register_test<test_runner_test_creation>();
    runner.register_test<test_runner_project_creation>();
    runner.register_test<test_runner_end_to_end_test>();
    runner.register_test<test_runner_project_files>();
    runner.register_test<versioned_output_free_test>();
    runner.register_test<file_editors_free_test>();
    runner.register_test<individual_test_paths_free_test>();
    runner.register_test<basic_test_interface_free_test>();
    runner.register_test<commands_free_test>();
    runner.register_test<failure_info_test>();
    runner.register_test<failure_info_false_negative_test>();
    runner.register_test<file_system_utilities_free_test>();
    runner.register_test<output_free_test>();
    runner.register_test<dependency_analyzer_free_test>();
    runner.register_test<materials_updater_free_test>();
    runner.register_test<free_checkers_meta_free_test>();
    runner.register_test<elementary_false_negative_free_diagnostics>();
    runner.register_test<elementary_false_positive_free_diagnostics>();
    runner.register_test<exceptions_false_negative_free_diagnostics>();
    runner.register_test<exceptions_false_positive_free_diagnostics>();
    runner.register_test<chrono_false_negative_free_diagnostics>();
    runner.register_test<chrono_false_positive_free_diagnostics>();
    runner.register_test<complex_false_negative_free_diagnostics>();
    runner.register_test<complex_false_positive_free_diagnostics>();
    runner.register_test<container_false_negative_free_diagnostics>();
    runner.register_test<container_false_positive_free_diagnostics>();
    runner.register_test<path_false_negative_free_diagnostics>();
    runner.register_test<path_false_positive_free_diagnostics>();
    runner.register_test<string_false_negative_free_diagnostics>();
    runner.register_test<string_false_positive_free_diagnostics>();
    runner.register_test<sum_types_false_negative_free_diagnostics>();
    runner.register_test<sum_types_false_positive_free_diagnostics>();
    runner.register_test<smart_pointer_false_negative_free_diagnostics>();
    runner.register_test<smart_pointer_false_positive_free_diagnostics>();
    runner.register_test<function_false_negative_free_diagnostics>();
    runner.register_test<function_false_positive_free_diagnostics>();
    runner.register_test<regular_false_negative_diagnostics>();
    runner.register_test<move_only_false_negative_diagnostics>();
    runner.register_test<orderable_move_only_false_negative_diagnostics>();
    runner.register_test<orderable_regular_false_negative_diagnostics>();
    runner.register_test<regular_false_positive_diagnostics>();
    runner.register_test<move_only_false_positive_diagnostics>();
    runner.register_test<orderable_move_only_false_positive_diagnostics>();
    runner.register_test<orderable_regular_false_positive_diagnostics>();
    runner.register_test<allocation_false_negative_diagnostics>();
    runner.register_test<allocation_false_negative_diagnostics_broken_semantics>();
    runner.register_test<allocation_false_negative_diagnostics_broken_value_semantics>();
    runner.register_test<allocation_false_negative_diagnostics_inefficient_operations>();
    runner.register_test<move_only_allocation_false_negative_diagnostics>();
    runner.register_test<allocation_false_positive_diagnostics>();
    runner.register_test<move_only_allocation_false_positive_diagnostics>();
    runner.register_test<scoped_allocation_false_negative_diagnostics>();
    runner.register_test<move_only_scoped_allocation_false_negative_diagnostics>();
    runner.register_test<scoped_allocation_false_positive_diagnostics>();
    runner.register_test<scoped_allocation_false_positive_diagnostics_mixed>();
    runner.register_test<scoped_allocation_false_positive_diagnostics_three_level>();
    runner.register_test<move_only_scoped_allocation_false_positive_diagnostics>();
    runner.register_test<orderable_move_only_allocation_false_positive_diagnostics>();
    runner.register_test<orderable_regular_allocation_false_positive_diagnostics>();
    runner.register_test<orderable_regular_allocation_false_negative_diagnostics>();
    runner.register_test<performance_false_negative_diagnostics>();
    runner.register_test<performance_false_positive_diagnostics>();
    runner.register_test<performance_utilities_test>();
    runner.register_test<relational_false_negative_diagnostics>();
    runner.register_test<relational_false_positive_diagnostics>();
    runner.register_test<regular_state_transition_false_negative_diagnostics>();
    runner.register_test<regular_state_transition_false_positive_diagnostics>();
    runner.register_test<move_only_state_transition_false_positive_diagnostics>();
    runner.register_test<move_only_state_transition_false_negative_diagnostics>();
    runner.register_test<indent_free_test>();
    runner.register_test<patterns_free_test>();
    runner.register_test<substitutions_free_test>();
    runner.register_test<commandline_arguments_false_negative_test>();
    runner.register_test<commandline_arguments_test>();
    runner.register_test<factory_false_negative_test>();
    runner.register_test<factory_test>();
    runner.register_test<invoke_handle_inheritance_free_test>();
    runner.register_test<invoke_interpreter_resolution_free_test>();
    runner.register_test<shell_commands_false_negative_test>();
    runner.register_test<shell_commands_test>();
    runner.register_test<type_name_free_test>();
    runner.register_test<sequences_free_test>();
    runner.register_test<type_algorithms_free_test>();
    runner.register_test<type_list_free_test>();
    runner.register_test<type_traits_test>();
    runner.register_test<concepts_test>();
    runner.register_test<utilities_test>();
    runner.register_test<creator_free_test>();
    runner.register_test<algorithms_test>();
    runner.register_test<statistical_algorithms_test>();
    runner.register_test<monotonic_sequence_false_negative_test>();
    runner.register_test<monotonic_sequence_test>();
    runner.register_test<monotonic_sequence_allocation_test>();
    runner.register_test<linear_sequence_false_negative_test>();
    runner.register_test<linear_sequence_test>();
    runner.register_test<array_utilities_test>();
    runner.register_test<iterator_test>();
    runner.register_test<threading_models_test>();
    runner.register_test<threading_models_performance_test>();
    runner.register_test<partitioned_data_false_negative_test>();
    runner.register_test<static_linearly_partitioned_sequence_false_negative_test>();
    runner.register_test<partition_iterator_test>();
    runner.register_test<bucketed_sequence_regular_test>();
    runner.register_test<bucketed_sequence_allocation_test>();
    runner.register_test<partitioned_sequence_regular_test>();
    runner.register_test<partitioned_sequence_allocation_test>();
    runner.register_test<static_partitioned_sequence_test>();
    runner.register_test<static_linearly_partitioned_sequence_test>();
    runner.register_test<test_static_stack_false_negatives>();
    runner.register_test<test_static_stack>();
    runner.register_test<test_static_queue_false_negatives>();
    runner.register_test<test_static_queue>();
    runner.register_test<test_static_priority_queue_false_negatives>();
    runner.register_test<test_static_priority_queue>();
    runner.register_test<test_graph_false_negatives>();
    runner.register_test<test_graph_meta>();
    runner.register_test<test_edge_false_negatives>();
    runner.register_test<test_edges>();
    runner.register_test<node_storage_test>();
    runner.register_test<node_storage_allocation_test>();
    runner.register_test<test_heterogeneous_node_storage>();
    runner.register_test<dynamic_directed_graph_unweighted_test>();
    runner.register_test<dynamic_directed_graph_unweighted_contiguous_test>();
    runner.register_test<dynamic_directed_graph_fundamental_weight_test>();
    runner.register_test<dynamic_directed_graph_fundamental_weight_contiguous_test>();
    runner.register_test<dynamic_undirected_graph_unweighted_test>();
    runner.register_test<dynamic_undirected_graph_unweighted_contiguous_test>();
    runner.register_test<dynamic_undirected_graph_fundamental_weight_test>();
    runner.register_test<dynamic_undirected_graph_fundamental_weight_contiguous_test>();
    runner.register_test<dynamic_undirected_graph_unsortable_weight_test>();
    runner.register_test<dynamic_undirected_graph_shared_fundamental_weight_test>();
    runner.register_test<dynamic_undirected_graph_shared_fundamental_weight_contiguous_test>();
    runner.register_test<dynamic_undirected_graph_shared_unsortable_weight_test>();
    runner.register_test<dynamic_undirected_graph_meta_data_test>();
    runner.register_test<dynamic_undirected_embedded_graph_unweighted_test>();
    runner.register_test<dynamic_undirected_embedded_graph_unweighted_contiguous_test>();
    runner.register_test<dynamic_undirected_embedded_graph_fundamental_weight_test>();
    runner.register_test<dynamic_undirected_embedded_graph_fundamental_weight_contiguous_test>();
    runner.register_test<dynamic_undirected_embedded_graph_shared_fundamental_weight_test>();
    runner.register_test<dynamic_undirected_embedded_graph_shared_fundamental_weight_contiguous_test>();
    runner.register_test<dynamic_undirected_embedded_graph_meta_data_test>();
    runner.register_test<static_directed_graph_unweighted_test>();
    runner.register_test<static_directed_graph_fundamental_weight_test>();
    runner.register_test<static_undirected_graph_unweighted_test>();
    runner.register_test<static_undirected_graph_fundamental_weight_test>();
    runner.register_test<static_undirected_graph_unsortable_weight_test>();
    runner.register_test<static_undirected_embedded_graph_unweighted_test>();
    runner.register_test<static_undirected_embedded_graph_fundamental_weight_test>();
    runner.register_test<test_heterogeneous_static_graph>();
    runner.register_test<weighted_graph_allocation_bucketed_test>();
    runner.register_test<weighted_graph_allocation_contiguous_test>();
    runner.register_test<unweighted_graph_allocation_bucketed_test>();
    runner.register_test<unweighted_graph_allocation_contiguous_test>();
    runner.register_test<test_graph_traversals>();
    runner.register_test<test_static_graph_traversals>();
    runner.register_test<test_graph_update>();
    runner.register_test<test_subgraph>();
    runner.register_test<experimental_test>();
    runner.register_test<flatten_type_list_free_test>();
    runner.register_test<streaming_free_test>();
    runner.register_test<tree_false_negative_test>();
    runner.register_test<tree_test>();
    runner.register_test<bitmask_free_test>();
    runner.register_test<file_system_free_test>();
    runner.register_test<normal_path_false_negative_test>();
    runner.register_test<normal_path_test>();
    runner.register_test<ratio_free_test>();
    runner.register_test<numeric_rings_meta_free_test>();
    runner.register_test<spaces_meta_free_test>();
    runner.register_test<bounds_free_test>();
    runner.register_test<validators_free_test>();
    runner.register_test<vector_coordinates_false_negative_test>();
    runner.register_test<vector_coordinates_test>();
    runner.register_test<complex_vector_coordinates_test>();
    runner.register_test<vector_polar_coordinates_test>();
    runner.register_test<affine_coordinates_false_negative_test>();
    runner.register_test<affine_coordinates_test>();
    runner.register_test<m_affine_coordinates_test>();
    runner.register_test<free_module_coordinates_test>();
    runner.register_test<partial_m_torsor_coordinates_test>();
    runner.register_test<absolute_coordinates_false_negative_test>();
    runner.register_test<absolute_coordinates_test>();
    runner.register_test<absolute_logarithmic_coordinates_test>();
    runner.register_test<space_ordering_meta_free_test>();
    runner.register_test<physical_value_meta_free_test>();
    runner.register_test<physical_value_false_negative_test>();
    runner.register_test<physical_value_conversions_free_test>();
    runner.register_test<absolute_physical_value_test>();
    runner.register_test<absolute_physical_value_compositions_test>();
    runner.register_test<unsafe_absolute_physical_value_test>();
    runner.register_test<unsafe_absolute_physical_value_compositions_test>();
    runner.register_test<affine_physical_value_test>();
    runner.register_test<convex_physical_value_test>();
    runner.register_test<vector_physical_value_test>();
    runner.register_test<vector_physical_value_compositions_test>();
    runner.register_test<mixed_physical_value_test>();
    runner.register_test<integral_physical_value_test>();
    runner.register_test<mem_ordered_tuple_false_negative_test>();
    runner.register_test<mem_ordered_tuple_test>();
    runner.register_test<saturating_mul_free_test>();
    runner.register_test<saturating_add_free_test>();
    runner.register_test<vector_nonlinear_representations_free_test>();
    runner.register_test<arithmetic_casts_free_test>();

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

