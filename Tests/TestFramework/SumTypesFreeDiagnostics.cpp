////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SumTypesFreeDiagnostics.hpp"
#include "ElementaryFreeDiagnosticsUtilities.hpp"

#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path sum_types_false_positive_free_diagnostics::source_file() const noexcept
  {
    return std::source_location::current().file_name();
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

      check(equality, report_line("std::variant holding different types but with the same value"), var{0}, var{0.0});
      check(equality, report_line("std::variant holding the zeroth type, but with different values"), var{1}, var{2});
      check(equality, report_line("std::variant holding the first type, but with different values"), var{-0.1}, var{0.0});

      check(equality,
            report_line("std::variant proffering advice for different values of the zeroth type"),
            var{-1},
            var{0},
            tutor{[](int, int) { return std::string{"int advice"}; }});

      check(equality,
            report_line("std::variant proffering advice for different values of the first type"),
            var{-0.1},
            var{0.0},
            tutor{[](double, double) { return std::string{"double advice"}; }});

      check(equality,
            report_line("std::variant advice ignored due to type mismatch"),
            var{-1.0},
            var{0.0},
            tutor{[](int, int) { return std::string{"Ignored advice, since double to int is a narrowing conversion"}; }});
    }

    {
      using var = std::variant<int, only_equivalence_checkable, only_weakly_checkable>;

      check(with_best_available, report_line(""), var{1}, var{2});
      check(with_best_available, report_line(""), var{only_equivalence_checkable{41}}, var{only_equivalence_checkable{42}});
      check(with_best_available, report_line(""), var{only_weakly_checkable{1, 1.5}}, var{only_weakly_checkable{1, 2.5}});

      check(with_best_available, report_line(""), var{1}, var{2}, tutor{[](int, int) { return "int advice"; }});
    }
  }

  void sum_types_false_positive_free_diagnostics::test_optional()
  {
    {
      using opt = std::optional<int>;

      check(equality, report_line("Empty vs non-empty std::optional"), opt{}, opt{0});
      check(equality, report_line("Non-empty vs empty std::optional"), opt{0}, opt{});
      check(equality, report_line("Two std::optionals holdings different values"), opt{2}, opt{0});

      check(equality,
            report_line("Advice for two std::optionals holdings different values"),
            opt{2},
            opt{0},
            tutor{[](int, int) { return "int advice"; }});
    }

    {
      using opt = std::optional<only_equivalence_checkable>;

      check(equivalence, report_line("Empty vs non-empty std::optional"), opt{}, opt{0});
      check(equivalence, report_line("Non-empty vs empty std::optional"), opt{0}, opt{});
      check(equivalence, report_line("Two std::optionals holdings different values"), opt{2}, opt{0});
    }

    {
      using opt = std::optional<only_weakly_checkable>;

      check(weak_equivalence, report_line("Empty vs non-empty std::optional"), opt{}, opt{{0, 0.0}});
      check(weak_equivalence, report_line("Non-empty vs empty std::optional"), opt{{0, 0.0}}, opt{});
      check(weak_equivalence, report_line("Two std::optionals holdings different values"), opt{{2, 1.0}}, opt{{1, 2.0}});
    }
  }

  void sum_types_false_positive_free_diagnostics::test_any()
  {
    check(equivalence, report_line("Empty std::any"), std::any{}, 1);
    check(equivalence, report_line("std::any holding the wrong type"), std::any{1},1.0);
    check(equivalence, report_line("std::any holding the wrong value"), std::any{1}, 2);

    check(equivalence, report_line(""), std::any{only_equivalence_checkable{1}}, only_equivalence_checkable{2});
    check(equivalence, report_line(""), std::any{only_weakly_checkable{1, 1.0}}, only_weakly_checkable{2, 2.0});

    check(equivalence,
          report_line("Advice for std::any holding the wrong value"),
          std::any{1},
          2,
          tutor{[](int, int) { return "int advice";}});
  }

  [[nodiscard]]
  std::filesystem::path sum_types_false_negative_free_diagnostics::source_file() const noexcept
  {
    return std::source_location::current().file_name();
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

      check(equality, report_line(""), var{0}, var{0});
      check(equality, report_line(""), var{0.0}, var{0.0});
      check(equality, report_line(""), var{3}, var{3});
      check(equality, report_line(""), var{4.0}, var{4.0});
      check(with_best_available, report_line(""), var{4.0}, var{4.0});
    }

    {
      using var = std::variant<int, only_equivalence_checkable, only_weakly_checkable>;

      check(with_best_available, report_line(""), var{1}, var{1});
      check(with_best_available, report_line(""), var{only_equivalence_checkable{1}}, var{only_equivalence_checkable{1}});
      check(with_best_available, report_line(""), var{only_weakly_checkable{1, 2.0}}, var{only_weakly_checkable{1, 2.0}});
    }
  }

  void sum_types_false_negative_free_diagnostics::test_optional()
  {
    {
      using opt = std::optional<int>;

      check(equality, report_line(""), opt{}, opt{});
      check(equality, report_line(""), opt{0}, opt{0});
      check(with_best_available, report_line(""), opt{-1}, opt{-1});
    }

    {
      using opt = std::optional<only_equivalence_checkable>;

      check(equivalence, report_line(""), opt{}, opt{});
      check(equivalence, report_line(""), opt{2}, opt{2});
      check(with_best_available, report_line(""), opt{2}, opt{2});
    }

    {
      using opt = std::optional<only_weakly_checkable>;

      check(weak_equivalence, report_line(""), opt{{0, 0.0}}, opt{{0, 0.0}});
      check(with_best_available, report_line(""), opt{{0, 0.0}}, opt{{0, 0.0}});
    }
  }

  void sum_types_false_negative_free_diagnostics::test_any()
  {
    check(equivalence, report_line(""), std::any{1}, 1);
    check(equivalence, report_line(""), std::any{only_equivalence_checkable{1}}, only_equivalence_checkable{1});
    check(equivalence, report_line(""), std::any{only_weakly_checkable{1, 1.0}}, only_weakly_checkable{1, 1.0});
  }
}
