////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MemOrderedTupleTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path mem_ordered_tuple_false_negative_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void mem_ordered_tuple_false_negative_test::run_tests()
  {
    datastructures::mem_ordered_tuple<int> x{42}, y{7};
    check(equivalence, "", x, std::tuple{31});
    check(equality, "", x, y);
  }
}
