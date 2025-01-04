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
  namespace
  {
    struct foo
    {
      int i{};

      [[nodiscard]]
      friend auto operator<=>(const foo&, const foo&) noexcept = default;

      friend std::ostream& operator<<(std::ostream&, const foo&);
    };

    // TO DO: move this and add more, once https://github.com/llvm/llvm-project/issues/121648 is fixed
    static_assert(!checkable_against<equality_check_t, test_mode::standard, foo, foo, int>);
  }
  
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
