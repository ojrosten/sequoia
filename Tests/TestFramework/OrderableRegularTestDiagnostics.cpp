////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "OrderableRegularTestDiagnostics.hpp"
#include "OrderableRegularTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path orderable_regular_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void orderable_regular_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void orderable_regular_false_negative_diagnostics::test_regular_semantics()
  {
    {
      using beast = orderable_regular_beast<int>;

      check_semantics("Broken check invariant", beast{1}, beast{2}, std::weak_ordering::equivalent);
      check_semantics("Broken check invariant", beast{1}, beast{2}, std::weak_ordering::greater);
    }

    {
      using beast = regular_broken_less<int>;

      check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = regular_broken_lesseq<int>;

      check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = regular_broken_greater<int>;

      check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = regular_broken_greatereq<int>;

      check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = regular_inverted_comparisons<int>;

      check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast = regular_broken_spaceship<int>;

      check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    }

    {
      using beast   = orderable_regular_beast<int>;
      using equiv_t = std::initializer_list<int>;

      check_semantics("Incorrect x value", beast{1}, beast{2}, equiv_t{2}, equiv_t{2}, std::weak_ordering::less);
      check_semantics("Incorrect y value", beast{1}, beast{2}, equiv_t{1}, equiv_t{3}, std::weak_ordering::less);
      check_semantics("Incorrect x value; mutator", beast{1}, beast{2}, equiv_t{2}, equiv_t{2}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });
      check_semantics("Incorrect y value; mutator", beast{1}, beast{2}, equiv_t{1}, equiv_t{3}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });
    }

    {
      using beast   = orderable_specified_moved_from_beast<int>;
      using equiv_t = std::vector<int>;
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, equiv_t{1}, equiv_t{}, std::weak_ordering::less);
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, equiv_t{1}, equiv_t{2},   beast{1},   beast{}, std::weak_ordering::less);

      check_semantics("Incorrect moved-from state post construction",
                      beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, equiv_t{1}, equiv_t{}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });

      check_semantics("Incorrect moved-from state post construction",
                      beast{1}, beast{2}, equiv_t{1}, equiv_t{2},   beast{1},   beast{}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });

      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, equiv_t{}, equiv_t{2}, std::weak_ordering::less);
      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, equiv_t{1}, equiv_t{2},   beast{},   beast{2}, std::weak_ordering::less);

      check_semantics("Incorrect moved-from state post assignment",
                      beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, equiv_t{}, equiv_t{2}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });

      check_semantics("Incorrect moved-from state post assignment",
                      beast{1}, beast{2}, equiv_t{1}, equiv_t{2},   beast{},   beast{2}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });
    }
  }

  [[nodiscard]]
  std::filesystem::path orderable_regular_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void orderable_regular_false_positive_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void orderable_regular_false_positive_diagnostics::test_regular_semantics()
  {
    using beast   = orderable_regular_beast<int>;
    using equiv_t = std::initializer_list<int>;

    check_semantics("", beast{1}, beast{2}, std::weak_ordering::less);
    check_semantics("", beast{2}, beast{1}, std::weak_ordering::greater);
    check_semantics("", beast{1}, beast{2}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });
    check_semantics("", beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, std::weak_ordering::less);
    check_semantics("", beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, std::weak_ordering::less, [](auto& b) { b.x.front() = 3; });
  }
}
