////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "OrderableMoveOnlyTestDiagnostics.hpp"
#include "OrderableMoveOnlyTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view orderable_move_only_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void orderable_move_only_false_positive_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void orderable_move_only_false_positive_diagnostics::test_regular_semantics()
  {
    {
      using beast = orderable_move_only_beast<int>;

      check_semantics(LINE("Broken check invariant"), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::equivalent);
      check_semantics(LINE("Broken check invariant"), beast{2}, beast{1}, beast{2}, beast{1}, std::weak_ordering::less);
      check_semantics(LINE("Broken check invariant"), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::greater);
    }

    {
      using beast = move_only_broken_less<int>;

      check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_lesseq<int>;

      check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_greater<int>;

      check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_greatereq<int>;

      check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_inverted_comparisons<int>;

      check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_spaceship<int>;

      check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }
  }

  [[nodiscard]]
  std::string_view orderable_move_only_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void orderable_move_only_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void orderable_move_only_false_negative_diagnostics::test_regular_semantics()
  {
    using beast = orderable_move_only_beast<int>;

    check_semantics(LINE(""), beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    check_semantics(LINE(""), beast{2}, beast{1}, beast{2}, beast{1}, std::weak_ordering::greater);
  }
}
