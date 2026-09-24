////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
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
    using namespace object;
    using namespace std::literals::chrono_literals;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       "  ",
                       {.source_folder{"sequoia"}, .main_cpp{"TestChamber/Local/TestChamberMain.cpp"}, .ancillary_main_cpps{{"TestAll/TestMain.cpp"}}, .common_includes{"TestCommon/TestIncludes.hpp"}}};

    code = runner.execute(timer_resolution{1ms});
  }
  catch(const std::exception& e)
  {
    std::cout << e.what() << '\n';
  }
  catch(...)
  {
    std::cout << "Unrecognized error\n"; 
  }
  
  return sequoia::testing::to_exit_code(code);
}

