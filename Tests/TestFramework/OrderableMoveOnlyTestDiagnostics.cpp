////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "OrderableMoveOnlyTestDiagnostics.hpp"
#include "OrderableMoveOnlyTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path orderable_move_only_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void orderable_move_only_false_negative_diagnostics::run_tests()
  {
    test_semantics();
  }

  void orderable_move_only_false_negative_diagnostics::test_semantics()
  {
    {
      using beast = orderable_move_only_beast<int>;

      check_semantics("Broken check invariant", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::equivalent);
      check_semantics("Broken check invariant", beast{2}, beast{1}, beast{2}, beast{1}, std::weak_ordering::less);
      check_semantics("Broken check invariant", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::greater);
    }

    {
      using beast = move_only_broken_less<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_lesseq<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_greater<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_greatereq<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_inverted_comparisons<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = move_only_broken_spaceship<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using binder = orderable_resource_binder;

      check_semantics("Incorrect moved-from state post construction", binder{1}, binder{2}, binder{1}, binder{2}, binder{3}, binder{1}, std::strong_ordering::less);
      check_semantics("Incorrect moved-from state post construction", []() { return binder{1}; }, []() {return binder{2}; }, binder{3}, binder{1}, std::strong_ordering::less);

      check_semantics("Incorrect moved-from state post assignment", binder{1}, binder{2}, binder{1}, binder{2}, binder{0}, binder{3}, std::strong_ordering::less);
      check_semantics("Incorrect moved-from state post assignment", []() { return binder{1}; }, []() {return binder{2}; }, binder{0}, binder{3}, std::strong_ordering::less);
    }
  }

  [[nodiscard]]
  std::filesystem::path orderable_move_only_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void orderable_move_only_false_positive_diagnostics::run_tests()
  {
    test_semantics();
  }

  void orderable_move_only_false_positive_diagnostics::test_semantics()
  {
    {
      using beast = orderable_move_only_beast<int>;

      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2}, std::weak_ordering::less);
      check_semantics("Function object syntax", []() { return beast{1}; }, []() { return beast{2}; }, std::weak_ordering::less);
      check_semantics("", beast{2}, beast{1}, beast{2}, beast{1}, std::weak_ordering::greater);
    }

    {
      using binder = orderable_resource_binder;

      check_semantics("Check moved-from state", binder{1}, binder{2}, binder{1}, binder{2}, binder{0}, binder{0},  std::strong_ordering::less);
      check_semantics("Check moved-from state", []() { return binder{1}; }, []() {return binder{2}; }, binder{0}, binder{0}, std::strong_ordering::less);
    }
  }
}
