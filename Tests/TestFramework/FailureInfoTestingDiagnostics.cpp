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
  std::filesystem::path failure_info_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void failure_info_false_negative_test::run_tests()
  {
    using namespace std::string_literals;

    failure_info x{}, y{4}, z{0, "foo"};
    check(equivalence, "", x, std::pair{1, ""s});
    check(equivalence, "", x, std::pair{0, "foo"s});
    check(equality, "check_index differs", x, y);
    check(equality, "message differs", x, z);
  }
}
