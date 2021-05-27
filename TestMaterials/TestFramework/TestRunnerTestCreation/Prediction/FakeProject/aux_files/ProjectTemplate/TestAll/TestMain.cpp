////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  try
  {
    using namespace sequoia;
    using namespace testing;
	using namespace std::literals::chrono_literals;

	const auto paths{project_paths{project_root(argc, argv)}};
    test_runner runner{argc, argv, "Oliver J. Rosten", paths, "\t"};
 
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

