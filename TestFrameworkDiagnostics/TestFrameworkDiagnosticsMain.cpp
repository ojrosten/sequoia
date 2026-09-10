////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestIncludes.hpp"

import std;

int main(int argc, char** argv)
{
  auto code{sequoia::testing::return_code::incomplete_run};

  try
  {
    using namespace sequoia;
    using namespace testing;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestFrameworkDiagnostics/TestFrameworkDiagnosticsMain.cpp"}, .ancillary_main_cpps{{"TestAll/TestMain.cpp"}}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.register_test<test_runner_false_negative_test>();
    runner.register_test<test_runner_test>();
    runner.register_test<test_runner_performance_test>();
    runner.register_test<test_runner_test_creation>();
    runner.register_test<test_runner_project_creation>();
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
    runner.register_test<commandline_arguments_false_negative_test>();
    runner.register_test<commandline_arguments_test>();
    runner.register_test<factory_false_negative_test>();
    runner.register_test<factory_test>();
    runner.register_test<shell_commands_false_negative_test>();
    runner.register_test<shell_commands_test>();
    runner.register_test<indent_free_test>();
    runner.register_test<patterns_free_test>();
    runner.register_test<substitutions_free_test>();

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

