////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "SumTypesFreeDiagnostics.hpp"
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
    using var = std::variant<int, double>;

    check(equality, LINE("std::variant holding different types but with the same value"), var{0}, var{0.0});
    check(equality, LINE("std::variant holding the zeroth type, but with different values"), var{1}, var{2});
    check(equality, LINE("std::variant holding the first type, but with different values"), var{-0.1}, var{0.0});
  }

  void sum_types_false_positive_free_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check(equality, LINE("Empty vs non-empty std::optional"), opt{}, opt{0});
    check(equality, LINE("Non-empty vs empty std::optional"), opt{0}, opt{});
    check(equality, LINE("Two std::optionals holdings different values"), opt{2}, opt{0});
  }

  void sum_types_false_positive_free_diagnostics::test_any()
  {
    check(equivalence, LINE("Empty std::any"), std::any{}, 1);
    check(equivalence, LINE("std::any holding the wrong type"), std::any{1},1.0);
    check(equivalence, LINE("std::any holding the wrong value"), std::any{1}, 2);
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
    using var = std::variant<int, double>;

    check(equality, LINE(""), var{0}, var{0});
    check(equality, LINE(""), var{0.0}, var{0.0});
    check(equality, LINE(""), var{3}, var{3});
    check(equality, LINE(""), var{4.0}, var{4.0});
  }

  void sum_types_false_negative_free_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check(equality, LINE(""), opt{}, opt{});
    check(equality, LINE(""), opt{0}, opt{0});
    check(equality, LINE(""), opt{-1}, opt{-1});
  }

  void sum_types_false_negative_free_diagnostics::test_any()
  {
    check(equivalence, LINE(""), std::any{1}, 1);
  }
}
