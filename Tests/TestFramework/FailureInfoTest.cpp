////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FailureInfoTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view failure_info_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void failure_info_test::run_tests()
  {
    failure_info x{}, y{1, "foo"};
    check_equivalence(LINE("Useful Description"), x, 0, "");
    check_equivalence(LINE("Useful Description"), y, 1, "foo");

    check_semantics(LINE("Useful Description"), x, y, std::weak_ordering::less);
  }
}
