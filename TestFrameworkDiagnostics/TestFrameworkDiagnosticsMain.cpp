////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
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
    using namespace testing;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       "  ",
                       {.main_cpp{"TestFrameworkDiagnostics/TestFrameworkDiagnosticsMain.cpp"}, .ancillary_main_cpps{{"TestAll/TestMain.cpp"}}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.add_test_suite(
      "Test Runner",
      test_runner_false_negative_test{},
      test_runner_test{},
      test_runner_performance_test{},
      test_runner_test_creation{},
      test_runner_project_creation{}
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
      "Text Processing",
      indent_free_test{},
      patterns_free_test{},
      substitutions_free_test{}
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

