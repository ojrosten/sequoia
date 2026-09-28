////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "ResetOnMoveTestingDiagnostics.hpp"

namespace sequoia::testing
{
  using namespace object;

  namespace
  {
    [[nodiscard]]
    int first() { return 1; }

    [[nodiscard]]
    int second() { return 2; }
  }

  [[nodiscard]]
  std::filesystem::path reset_on_move_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void reset_on_move_false_negative_test::run_tests()
  {
    using handle          = reset_on_move<int, -1>;
    using function_handle = reset_on_move<int(*)()>;

    check(equality, "Unequal values", handle{7}, handle{8});
    check(equivalence, "Inequivalent value", handle{7}, 8);
    check(equality, "Unequal function pointers", function_handle{&first}, function_handle{&second});
    check(equivalence, "Inequivalent function pointer", function_handle{&first}, &second);
  }
}
