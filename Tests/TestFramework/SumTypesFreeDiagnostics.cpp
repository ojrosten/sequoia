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
  }


  void sum_types_false_positive_free_diagnostics::test_variant()
  {
    using var = std::variant<int, double>;

    check(equality, LINE(""), var{0}, var{0.0});
    check(equality, LINE(""), var{1}, var{2});
    check(equality, LINE(""), var{-0.1}, var{0.0});
  }

  void sum_types_false_positive_free_diagnostics::test_optional()
  {
    using opt = std::optional<int>;
    check(equality, LINE(""), opt{}, opt{0});
    check(equality, LINE(""), opt{0}, opt{});
    check(equality, LINE(""), opt{2}, opt{0});
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
}
