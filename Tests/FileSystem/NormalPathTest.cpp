////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "NormalPathTest.hpp"

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  [[nodiscard]]
  std::filesystem::path normal_path_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void normal_path_test::run_tests()
  {
    normal_path x{"A/C/../B"}, y{"A/C"};
    check(equivalence, "", x, fs::path{"A/B"});
    check(equivalence, "", y, fs::path{"A/C"});

    check_semantics("", x, y, std::weak_ordering::less);
  }
}
