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
                       {.main_cpp{"TestFrameworkHarness/TestFrameworkHarnessMain.cpp"}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    runner.register_test<test_runner_end_to_end_test>();
    runner.register_test<test_runner_project_files>();

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

