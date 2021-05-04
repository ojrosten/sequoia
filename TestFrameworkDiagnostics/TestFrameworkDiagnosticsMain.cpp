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
  using namespace sequoia;
  using namespace testing;

  try
  {
    const auto root{project_root(argc, argv)};

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       root/"TestFrameworkDiagnostics"/"TestFrameworkDiagnosticsMain.cpp",
                       root/"TestCommon" /"TestIncludes.hpp",
                       repositories{root}
    };

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
      "Extended Allocation Diagnostics",
      orderable_move_only_allocation_false_negative_diagnostics{"Orderable Move-Only Allocation False Negative Diagnostics"},
      orderable_regular_allocation_false_negative_diagnostics{"Orderable Regular Allocation False Negative Diagnostics"},
      orderable_regular_allocation_false_positive_diagnostics{"Orderable Regular Allocation False Positive Diagnostics"}
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
      "Output",
      output_free_test("Free Test")
    );

    runner.add_test_family(
      "Test Framework Auxiliary",
      file_system_free_test("File System Free Test")
    );

    runner.add_test_family(
      "Test Runner",
      test_runner_false_positive_test{"False Positive Diagnostics"},
      test_runner_test("Functionality Test"),
      test_runner_test_creation{"Test Creation"},
      test_runner_project_creation{"Project Creation"}
    );

    runner.add_test_family(
      "Text Processing",
      indent_free_test("Indent Free Test"),
      patterns_free_test("Patterns Free Test"),
      substitutions_free_test("Substitutions Free Test")
    );

    using namespace std::literals::chrono_literals;
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

