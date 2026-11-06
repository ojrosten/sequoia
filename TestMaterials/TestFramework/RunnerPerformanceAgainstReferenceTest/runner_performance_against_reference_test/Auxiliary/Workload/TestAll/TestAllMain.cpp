////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "WorkloadTests.hpp"

#include "sequoia/TestFramework/TestRunner.hpp"

#include <iostream>

int main(int argc, char** argv)
{
  auto code{sequoia::testing::return_code::incomplete_run};

  try
  {
    using namespace workload;

    sequoia::testing::test_runner runner{argc, argv, "Oliver J. Rosten", "  "};

    runner.register_test<workload_test_0>();
    runner.register_test<workload_test_1>();
    runner.register_test<workload_test_2>();
    runner.register_test<workload_test_3>();
    runner.register_test<workload_test_4>();
    runner.register_test<workload_test_5>();
    runner.register_test<workload_test_6>();
    runner.register_test<workload_test_7>();

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
