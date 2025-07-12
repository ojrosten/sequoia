////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MemOrderedTupleTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path mem_ordered_tuple_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void mem_ordered_tuple_test::run_tests()
  {
    using namespace datastructures;
    check_semantics("", mem_ordered_tuple<int>{42}, mem_ordered_tuple<int>{7}, std::tuple{42}, std::tuple{7});
  }
}
