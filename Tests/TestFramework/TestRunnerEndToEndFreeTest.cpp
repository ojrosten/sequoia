////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerEndToEndFreeTest.hpp"
#include "TestRunner.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_end_to_end_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_end_to_end_test::run_tests()
  {
    // TO DO
  }
}
