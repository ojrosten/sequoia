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

    const auto root{project_root(argc, argv)};
    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       project_paths{root,
                                     root / "TestChamber" / "TestChamberMain.cpp",
                                     root / "TestCommon" / "TestIncludes.hpp"}
    };

    runner.add_test_family(
      "Experimental",
      experimental_test{"Unit Test"}
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

