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
    using namespace object;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       {"TestChamber/TestChamberMain.cpp", {}, "TestCommon/TestIncludes.hpp"}};

    runner.add_test_suite(
      "Experimental",
      experimental_test{"Unit Test"}
    );

    runner.add_test_suite(
      "Partitioned Data",
      partitioned_data_false_positive_test{"False Positive Diagnostics"},
      partitioned_data_test{"Unit Test"},
      bucketed_sequence_regular_test{"Bucketed Sequence Regular Test"},
      partitioned_sequence_regular_test{"Partitioned Sequence Regular Test"},
      partitioned_data_allocation_test{"Allocation Test"}
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

