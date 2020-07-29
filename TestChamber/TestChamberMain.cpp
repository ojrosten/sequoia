////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
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
    test_runner runner{argc, argv};
  
    runner.add_test_family(
      "Diagnostics",
      false_positive_diagnostics{"False Positive Diagnostics"},
      move_only_false_positive_diagnostics{"Move-Only False Positive Diagnostics"},
      performance_false_positive_diagnostics{"Performance False Positive Diagnostics"},
      fuzzy_false_positive_diagnostics{"Fuzzy False Positive Diagnostics"},
      allocation_false_positive_diagnostics{"Alloction False Positive Diagnostics"},
      move_only_allocation_false_positive_diagnostics{"Move-Only Alloction False Positive Diagnostics"},
      scoped_allocation_false_positive_diagnostics{"Scoped Allocation False Negative Diagnostics"},
      move_only_scoped_allocation_false_positive_diagnostics{"Move-Only Scoped Allocation False Negative Diagnostics"},
      false_negative_diagnostics{"False Negative Diagnostics"},
      move_only_false_negative_diagnostics{"Move-Only False Negative Diagnostics"},
      performance_false_negative_diagnostics{"Performance False Negative Diagnostics"},
      fuzzy_false_negative_diagnostics{"Fuzzy False Negative Diagnostics"},
      allocation_false_negative_diagnostics{"Allocation False Negative Diagnostics"},
      move_only_allocation_false_negative_diagnostics{"Move-Only Allocation False Negative Diagnostics"},
      scoped_allocation_false_negative_diagnostics{"Scoped Allocation False Negative Diagnostics"},
      move_only_scoped_allocation_false_negative_diagnostics{"Move-Only Scoped Allocation False Negative Diagnostics"}
    );

    runner.add_test_family(
      "CommandLine Arguments",
      commandline_arguments_test{"Unit Test"},
      commandline_arguments_false_positive_test{"False Positive Test"}
    );

    runner.add_test_family(
      "Partitioned Data",
      partitioned_data_false_positive_test{"False Positive Diagnostics"}
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

