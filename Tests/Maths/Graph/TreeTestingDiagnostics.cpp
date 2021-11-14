////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TreeTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace maths;

  [[nodiscard]]
  std::string_view tree_false_positive_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void tree_false_positive_test::run_tests()
  {
    tree<directed_flavour::directed, tree_link_direction::forward, null_weight, int> x{}, y{{1}};

    // check_equivalence(LINE("Useful Description"), x, something inequivalent - ordinarily this would fail);

    check_equality(LINE(""), x, y);
  }
}
