////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerDiagnostics.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_runner_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_runner_false_positive_test::run_tests()
  {
    test_template_data_generation();
  }

  void test_runner_false_positive_test::test_template_data_generation()
  {
    check_equality(LINE("Wrong species"),
                   generate_template_data("<class T>"), template_data{{"typename", "T"}});

    check_equality(LINE("Wrong symbol"),
                   generate_template_data("<class S>"), template_data{{"class", "T"}});
  }
}
