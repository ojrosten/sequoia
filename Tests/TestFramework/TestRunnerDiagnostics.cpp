////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "TestRunnerDiagnostics.hpp"
#include "TestRunnerDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path test_runner_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void test_runner_false_negative_test::run_tests()
  {
    test_template_data_generation();
  }

  void test_runner_false_negative_test::test_template_data_generation()
  {
    check(equality, "Wrong species",
                   generate_template_data("<class T>"), template_data{{"typename", "T"}});

    check(equality, "Wrong symbol",
                   generate_template_data("<class S>"), template_data{{"class", "T"}});
  }
}
