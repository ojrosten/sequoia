////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FailureInfoTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path failure_info_false_positive_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void failure_info_false_positive_test::run_tests()
  {
    failure_info x{}, y{4}, z{0, "foo"};
    check(equivalence, report_line(""), x, 1, "");
    check(equivalence, report_line(""), x, 0, "foo");
    check(equality, report_line("check_index differs"), x, y);
    check(equality, report_line("message differs"), x, z);
  }
}
