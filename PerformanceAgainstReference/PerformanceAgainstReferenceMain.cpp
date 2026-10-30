////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestFramework/RunnerPerformanceAgainstReferenceTest.hpp"

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  auto code{sequoia::testing::return_code::incomplete_run};

  try
  {
    using namespace sequoia;
    using namespace testing;

    test_runner runner{argc,
                       argv,
                       "Oliver J. Rosten",
                       "  ",
                       {.source_folder{"sequoia"},
                        .main_cpp{"PerformanceAgainstReference/PerformanceAgainstReferenceMain.cpp"},
                        .common_includes{"PerformanceAgainstReference/PerformanceAgainstReferenceMain.cpp"}}};

    runner.register_test<runner_performance_against_reference_test>();

    code = runner.execute();
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
