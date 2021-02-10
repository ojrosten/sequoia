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
                       root/"TestMinimal"/"TestMinimalMain.cpp",
                       root/"TestCommon" /"TestIncludes.hpp",
                       repositories(root)
    };
  
    /*
    runner.add_test_family(
      "Core Diagnostics",
      false_positive_diagnostics{"False Positive Diagnostics"},
      false_negative_diagnostics{"False Negative Diagnostics"}
    );

    runner.add_test_family(
      "Test Runner",      
      test_runner_false_positive_test{"False Positive Diagnostics"},
      test_runner_test_creation{"Test Creation"},
      test_runner_project_creation{"Project Creation"}
    );

    runner.add_test_family(
      "CommandLine Arguments",      
      commandline_arguments_false_positive_test{"False Positive Test"},
      commandline_arguments_test{"Unit Test"}
      );
    */
      
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

