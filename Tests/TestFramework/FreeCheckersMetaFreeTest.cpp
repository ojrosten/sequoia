////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "FreeCheckersMetaFreeTest.hpp"
#include "sequoia/TestFramework/FreeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path free_checkers_meta_free_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void free_checkers_meta_free_test::run_tests()
  {
    check("", [](){ static_assert(is_customized_check<general_equivalence_check_t<int>>); return true; }());
    check("", [](){ static_assert(is_customized_check<general_weak_equivalence_check_t<int>>); return true; }());
  }
}
