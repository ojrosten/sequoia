////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2025.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

namespace generatedProject::testing{};

int main(int argc, char** argv)
{
  try
  {
    using namespace generatedProject::testing;
    using namespace std::literals::chrono_literals;

    sequoia::testing::test_runner runner{argc, argv, "Oliver Jacob Rosten", "  "};

    runner.execute(sequoia::timer_resolution{1ms});
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

