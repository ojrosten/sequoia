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
      test_runner_false_negative_test{"False Negative Diagnostics"},
      test_runner_test{"Functionality Test"},
      test_runner_performance_test{"Test Runner Performance Test"},
      test_runner_test_creation{"Test Creation"},
      test_runner_project_creation{"Project Creation"}
    );

    runner.add_test_suite(
      "Test Framework Auxiliary",
      individual_test_paths_free_test{"Individual Test Paths Free Test"},
      basic_test_interface_free_test{"Basic Test Interface Free Test"},
      commands_free_test{"Commands Free Test"},
      failure_info_test{"failure_info Unit Test"},
      failure_info_false_negative_test{"failure_info False Negative Test"},
      file_system_utilities_free_test{"File System Free Test"},
      output_free_test{"Output Free Test"},
      dependency_analyzer_free_test{"Dependency Analyzer Free Test"},
      materials_updater_free_test{"Free Test"}
    );

    runner.add_test_suite(
      "Core Diagnostics",
      free_checkers_meta_free_test{"Free Checkers Meta Free Test"},
      elementary_false_negative_free_diagnostics{"Elementary False Negative Free Diagnostics"},
      elementary_false_positive_free_diagnostics{"Elementary False Positive Free Diagnostics"},
      exceptions_false_negative_free_diagnostics{"Exceptions False Negative Free Diagnostics"},
      exceptions_false_positive_free_diagnostics{"Exceptions False Positive Free Diagnostics"},
      chrono_false_negative_free_diagnostics{"Chrono False Negative Free Diagnostics"},
      chrono_false_positive_free_diagnostics{"Chrono False Positive Free Diagnostics"},
      complex_false_negative_free_diagnostics{"Complex False Negative Free Diagnostics"},
      complex_false_positive_free_diagnostics{"Complex False Positive Free Diagnostics"},
      container_false_negative_free_diagnostics{"Container False Negative Free Diagnostics"},
      container_false_positive_free_diagnostics{"Container False Positive Free Diagnostics"},
      path_false_negative_free_diagnostics{"Path False Negative Free Diagnostics"},
      path_false_positive_free_diagnostics{"Path False Positive Free Diagnostics"},
      string_false_negative_free_diagnostics{"String False Negative Free Diagnostics"},
      string_false_positive_free_diagnostics{"String False Positive Free Diagnostics"},
      sum_types_false_negative_free_diagnostics{"Sum Types False Negative Free Diagnostics"},
      sum_types_false_positive_free_diagnostics{"Sum Types False Positive Free Diagnostics"},
      smart_pointer_false_negative_free_diagnostics{"Smart Pointer False Negative Free Diagnostics"},
      smart_pointer_false_positive_free_diagnostics{"Smart Pointer False Positive Free Diagnostics"},
      function_false_negative_free_diagnostics{"Function False Negative Free Diagnostics"},
      function_false_positive_free_diagnostics{"Function False Positive Free Diagnostics"}
    );

    runner.add_test_suite(
      "Semantics Testing Diagnostics",
      regular_false_negative_diagnostics{"Regular False Negative Diagnostics"},
      move_only_false_negative_diagnostics{"Move-Only False Negative Diagnostics"},
      orderable_move_only_false_negative_diagnostics{"Orderable Move-Only False Negative Diagnostics"},
      orderable_regular_false_negative_diagnostics{"Orderable Regular False Negative Diagnostics"},
      regular_false_positive_diagnostics{"Regular False Positive Diagnostics"},
      move_only_false_positive_diagnostics{"Move-Only False Positive Diagnostics"},
      orderable_move_only_false_positive_diagnostics{"Orderable Move-Only False Positive Diagnostics"},
      orderable_regular_false_positive_diagnostics{"Orderable Regular False Positive Diagnostics"}
    );

    runner.add_test_suite(
      "Allocation Diagnostics",
      allocation_false_negative_diagnostics{"Allocation False Negative Diagnostics"},
      allocation_false_negative_diagnostics_broken_semantics{"Allocation False Negative Diagnostics: Broken Semantics"},
      allocation_false_negative_diagnostics_broken_value_semantics{"Allocation False Negative Diagnostics: Broken Value Semantics"},
      allocation_false_negative_diagnostics_inefficient_operations{"Allocation False Negative Diagnostics: Inefficient Operations"},
      move_only_allocation_false_negative_diagnostics{"Move-Only Alloction False Negative Diagnostics"},
      allocation_false_positive_diagnostics{"Allocation False Positive Diagnostics"},
      move_only_allocation_false_positive_diagnostics{"Move-Only Allocation False Positive Diagnostics"}
    );

    runner.add_test_suite(
      "Scoped Allocation Diagnostics",
      scoped_allocation_false_negative_diagnostics{"Scoped Allocation False Negative Diagnostics"},
      move_only_scoped_allocation_false_negative_diagnostics{"Move-Only Scoped Allocation False Negative Diagnostics"},
      scoped_allocation_false_positive_diagnostics{"Scoped Allocation False Positive Diagnostics"},
      scoped_allocation_false_positive_diagnostics_mixed{"Scoped Allocation False Positive Diagnostics: Mixed"},
      scoped_allocation_false_positive_diagnostics_three_level{"Scoped Allocation False Positive Diagnostics: Three Level"},
      move_only_scoped_allocation_false_positive_diagnostics{"Move-Only Scoped Allocation False Positive Diagnostics"}
    );

    runner.add_test_suite(
      "Extended Allocation Diagnostics",
      orderable_move_only_allocation_false_positive_diagnostics{"Orderable Move-Only Allocation False Positive Diagnostics"},
      orderable_regular_allocation_false_positive_diagnostics{"Orderable Regular Allocation False Positive Diagnostics"},
      orderable_regular_allocation_false_negative_diagnostics{"Orderable Regular Allocation False Negative Diagnostics"}
    );

    runner.add_test_suite(
      "Performance Diagnostics",
      performance_false_negative_diagnostics{"Performance False Negative Diagnostics"},
      performance_false_positive_diagnostics{"Performance False Positive Diagnostics"},
      performance_utilities_test{"Performance Utilities"}
    );

    runner.add_test_suite(
      "Relational Diagnostics",
      relational_false_negative_diagnostics{"Relational False Negative Diagnostics"},
      relational_false_positive_diagnostics{"Relational False Positive Diagnostics"}
    );

    runner.add_test_suite(
      "State Transition Utilities",
      regular_state_transition_false_negative_diagnostics{"Regular False Negative Diagnostics"},
      regular_state_transition_false_positive_diagnostics{"Regular False Positive Diagnostics"},
      move_only_state_transition_false_positive_diagnostics{"Move-Only False Negative Diagnostics"},
      move_only_state_transition_false_negative_diagnostics{"Move-Only False Positive Diagnostics"}
    );

    runner.add_test_suite(
      "CommandLine Arguments",
      commandline_arguments_false_negative_test{"False Negative Test"},
      commandline_arguments_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Factory",
      factory_false_negative_test{"False Negative Test"},
      factory_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Shell Commands",
      shell_commands_false_negative_test{"False Negative Test"},
      shell_commands_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Text Processing",
      indent_free_test{"Indent Free Test"},
      patterns_free_test{"Patterns Free Test"},
      substitutions_free_test{"Substitutions Free Test"}
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

