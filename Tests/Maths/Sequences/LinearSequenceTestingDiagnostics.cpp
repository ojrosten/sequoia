////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "LinearSequenceTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path linear_sequence_false_positive_test::source_file() const noexcept
  {
    return std::source_location::current().file_name();
  }

  void linear_sequence_false_positive_test::run_tests()
  {
    using namespace maths;

    linear_sequence<int, std::size_t> s{1, 3};
    check(equivalence, report_line(""), s, 0, 2);
  }
}
