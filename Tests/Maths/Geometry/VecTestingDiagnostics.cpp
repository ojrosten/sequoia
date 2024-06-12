////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "VecTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path vec_false_positive_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void vec_false_positive_test::run_tests()
  {
    // For example:

    // maths::vec<VectorSpace> x{args}, y{different args};
    // check(equivalence, report_line("Useful Description"), x, something inequivalent - ordinarily this would fail);
    // check(equality, report_line("Useful Description"), x, y);
  }
}
