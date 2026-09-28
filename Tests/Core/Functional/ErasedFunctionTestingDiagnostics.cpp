////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/** \file */

#include "ErasedFunctionTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path erased_function_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void erased_function_false_negative_test::run_tests()
  {
    constexpr observed_function empty{};
    const observed_function seven{[]() { return 7; }}, eight{[]() { return 8; }};

    check(equality, "One engaged, one empty", seven, empty);
    check(equality, "Different results", seven, eight);
    check(equivalence, "Engaged, predicted empty", seven, std::optional<int>{});
    check(equivalence, "Empty, predicted a result", empty, std::optional<int>{7});
    check(equivalence, "Different result", seven, std::optional<int>{8});
  }
}
