////////////////////////////////////////////////////////////////////
//               Copyright Oliver Jacob Rosten 2026.              //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

import std;
import sequoia.test_framework;

namespace generatedProject::testing{};

int main(int argc, char** argv)
{
  auto code{sequoia::testing::return_code::incomplete_run};

  try
  {
    using namespace generatedProject::testing;
    using namespace std::literals::chrono_literals;

    sequoia::testing::test_runner runner{argc, argv, "Oliver Jacob Rosten", "  "};

    code = runner.execute(sequoia::timer_resolution{1ms});
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

