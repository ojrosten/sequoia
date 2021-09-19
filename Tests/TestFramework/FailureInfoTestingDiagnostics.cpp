////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FailureInfoTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view failure_info_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void failure_info_false_positive_test::run_tests()
  {
    failure_info x{}, y{4}, z{0, "foo"};
    check_equivalence(LINE(""), x, 1, "");
    check_equivalence(LINE(""), x, 0, "foo");
    check_equality(LINE("check_index differs"), x, y);
    check_equality(LINE("message differs"), x, z);
  }
}
