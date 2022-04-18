////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SumTypesFreeDiagnostics.hpp"
#include "CoreDiagnosticsUtilities.hpp"

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view sum_types_false_positive_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void sum_types_false_positive_free_diagnostics::run_tests()
  {
    test_variant();
    test_optional();
    test_any();
  }


  void sum_types_false_positive_free_diagnostics::test_variant()
  {
    {
      using var = std::variant<int, double>;

      check(equality, LINE("std::variant holding different types but with the same value"), var{0}, var{0.0});
      check(equality, LINE("std::variant holding the zeroth type, but with different values"), var{1}, var{2});
      check(equality, LINE("std::variant holding the first type, but with different values"), var{-0.1}, var{0.0});
    }

    {
      using var = std::variant<int, only_equivalence_checkable, only_weakly_checkable>;

      check(with_best_available, LINE(""), var{1}, var{2});
      check(with_best_available, LINE(""), var{only_equivalence_checkable{41}}, var{only_equivalence_checkable{42}});
      check(with_best_available, LINE(""), var{only_weakly_checkable{1, 1.5}}, var{only_weakly_checkable{1, 2.5}});
    }
  }

  void sum_types_false_positive_free_diagnostics::test_optional()
  {
    {
      using opt = std::optional<int>;

      check(equality, LINE("Empty vs non-empty std::optional"), opt{}, opt{0});
      check(equality, LINE("Non-empty vs empty std::optional"), opt{0}, opt{});
      check(equality, LINE("Two std::optionals holdings different values"), opt{2}, opt{0});
    }

    {
      using opt = std::optional<only_equivalence_checkable>;

      check(equivalence, LINE("Empty vs non-empty std::optional"), opt{}, opt{0});
      check(equivalence, LINE("Non-empty vs empty std::optional"), opt{0}, opt{});
      check(equivalence, LINE("Two std::optionals holdings different values"), opt{2}, opt{0});
    }

    {
      using opt = std::optional<only_weakly_checkable>;

      check(weak_equivalence, LINE("Empty vs non-empty std::optional"), opt{}, opt{{0, 0.0}});
      check(weak_equivalence, LINE("Non-empty vs empty std::optional"), opt{{0, 0.0}}, opt{});
      check(weak_equivalence, LINE("Two std::optionals holdings different values"), opt{{2, 1.0}}, opt{{1, 2.0}});
    }
  }

  void sum_types_false_positive_free_diagnostics::test_any()
  {
    check(equivalence, LINE("Empty std::any"), std::any{}, 1);
    check(equivalence, LINE("std::any holding the wrong type"), std::any{1},1.0);
    check(equivalence, LINE("std::any holding the wrong value"), std::any{1}, 2);

    check(equivalence, LINE(""), std::any{only_equivalence_checkable{1}}, only_equivalence_checkable{2});
    check(equivalence, LINE(""), std::any{only_weakly_checkable{1, 1.0}}, only_weakly_checkable{2, 2.0});
  }

  [[nodiscard]]
  std::string_view sum_types_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void sum_types_false_negative_free_diagnostics::run_tests()
  {
    test_variant();
    test_optional();
    test_any();
  }

  void sum_types_false_negative_free_diagnostics::test_variant()
  {
    {
      using var = std::variant<int, double>;

      check(equality, LINE(""), var{0}, var{0});
      check(equality, LINE(""), var{0.0}, var{0.0});
      check(equality, LINE(""), var{3}, var{3});
      check(equality, LINE(""), var{4.0}, var{4.0});
      check(with_best_available, LINE(""), var{4.0}, var{4.0});
    }

    {
      using var = std::variant<int, only_equivalence_checkable, only_weakly_checkable>;

      check(with_best_available, LINE(""), var{1}, var{1});
      check(with_best_available, LINE(""), var{only_equivalence_checkable{1}}, var{only_equivalence_checkable{1}});
      check(with_best_available, LINE(""), var{only_weakly_checkable{1, 2.0}}, var{only_weakly_checkable{1, 2.0}});
    }
  }

  void sum_types_false_negative_free_diagnostics::test_optional()
  {
    {
      using opt = std::optional<int>;

      check(equality, LINE(""), opt{}, opt{});
      check(equality, LINE(""), opt{0}, opt{0});
      check(with_best_available, LINE(""), opt{-1}, opt{-1});
    }

    {
      using opt = std::optional<only_equivalence_checkable>;

      check(equivalence, LINE(""), opt{}, opt{});
      check(equivalence, LINE(""), opt{2}, opt{2});
      check(with_best_available, LINE(""), opt{2}, opt{2});
    }

    {
      using opt = std::optional<only_weakly_checkable>;

      check(weak_equivalence, LINE(""), opt{{0, 0.0}}, opt{{0, 0.0}});
      check(with_best_available, LINE(""), opt{{0, 0.0}}, opt{{0, 0.0}});
    }
  }

  void sum_types_false_negative_free_diagnostics::test_any()
  {
    check(equivalence, LINE(""), std::any{1}, 1);
    check(equivalence, LINE(""), std::any{only_equivalence_checkable{1}}, only_equivalence_checkable{1});
    check(equivalence, LINE(""), std::any{only_weakly_checkable{1, 1.0}}, only_weakly_checkable{1, 1.0});
  }
}
