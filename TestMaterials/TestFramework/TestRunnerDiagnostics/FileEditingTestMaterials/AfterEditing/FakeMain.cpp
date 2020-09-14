////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "Includes.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  using namespace sequoia;
  using namespace testing;

  try
  {
    runner.add_test_family(
      "CommandLine Arguments",
      commandline_arguments_test{"Unit Test"},
      commandline_arguments_false_positive_test{"False Positive Test"}
    );
      

    runner.add_test_family(
      "New Family",
      fake_false_positive_test{"False Positive Test"},
      fake_test{"Unit Test"}
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
