////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FuzzyTestDiagnostics.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view fuzzy_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void fuzzy_false_positive_diagnostics::run_tests()
  {
    basic_tests();
    range_tests();
    container_tests();
  }

  void fuzzy_false_positive_diagnostics::basic_tests()
  {
    check_approx_equality(LINE(""), within_tolerance{1.0}, 3.0, 5.0);
    check_approx_equality(LINE(""), within_tolerance{1.0}, 7.0, 5.0,
                          tutor{[](double, double){
                            return "Tweak your tolerance!";
                           }});

    check_approx_equality(LINE("<"),  inequality<int, std::less<int>>{},          5, 4);
    check_approx_equality(LINE("<="), inequality<int, std::less_equal<int>>{},    5, 4);
    check_approx_equality(LINE(">"),  inequality<int, std::greater<int>>{},       4, 5);
    check_approx_equality(LINE(">="), inequality<int, std::greater_equal<int>>{}, 4, 5);

    check_approx_equality(LINE(""), [](const double& lhs, const double& rhs){
        return std::abs(lhs - rhs) < 0.3;
      }, 1.0, 2.0);
  }

  void fuzzy_false_positive_diagnostics::range_tests()
  {
    {
      std::vector<double> v{0.5, 0.6}, p{-0.1, 1.0};
      check_range_approx(LINE(""), within_tolerance{0.5}, v.cbegin(), v.cend(), p.cbegin(), p.cend());
      check_range_approx(LINE(""), within_tolerance{0.5}, v.cbegin(), v.cend(), p.cbegin(), p.cend(),
        tutor{[](double, double) {
          return "Consider increasing tolerance!";
        }});

      p = {0.5, 1.2};
      check_range_approx(LINE(""), within_tolerance{0.5}, v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }

    {
      std::vector<int> v{4, 5}, p{5, 4};
      check_range_approx(LINE("<"), inequality<int, std::less<int>>{}, v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }
  }

  void fuzzy_false_positive_diagnostics::container_tests()
  {
    check_approx_equality(LINE(""), within_tolerance{0.5}, std::vector<double>{2.2, -1.0}, std::vector<double>{2.1, -1.8});

    using comp_t = std::vector<std::vector<double>>;
    check_approx_equality(LINE(""), within_tolerance{0.5}, comp_t{{2.2, -1.0}, {5.1}}, comp_t{{2.1, -1.1}, {3.7}});
  }

  [[nodiscard]]
  std::string_view fuzzy_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void fuzzy_false_negative_diagnostics::run_tests()
  {
    basic_tests();
    range_tests();
    container_tests();
  }

  void fuzzy_false_negative_diagnostics::basic_tests()
  {
    check_approx_equality(LINE(""), within_tolerance{1.0}, 4.5, 5.0);
    check_approx_equality(LINE(""), within_tolerance{1.0}, 5.5, 5.0);

    check_approx_equality(LINE(""), within_tolerance{0.5}, 4.5, 5.0);
    check_approx_equality(LINE(""), within_tolerance{0.5}, 5.5, 5.0);

    check_approx_equality(LINE("<"),  inequality<int, std::less<int>>{},          4, 5);
    check_approx_equality(LINE("<="), inequality<int, std::less_equal<int>>{},    4, 5);
    check_approx_equality(LINE(">"),  inequality<int, std::greater<int>>{},       5, 4);
    check_approx_equality(LINE(">="), inequality<int, std::greater_equal<int>>{}, 5, 4);
  }

  void fuzzy_false_negative_diagnostics::range_tests()
  {
    {
      std::vector<double> v{0.5, 0.6}, p{0, 1.0};
      check_range_approx(LINE(""), within_tolerance{0.5}, v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }

    {
      std::vector<int> v{4, 3}, p{5, 4};
      check_range_approx(LINE("<"), inequality<int, std::less<int>>{}, v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }
  }

  void fuzzy_false_negative_diagnostics::container_tests()
  {
    check_approx_equality(LINE(""), within_tolerance{0.5}, std::vector<double>{2.2, -1.0}, std::vector<double>{2.1, -1.1});

    using comp_t = std::vector<std::vector<double>>;
    check_approx_equality(LINE(""), within_tolerance{0.5}, comp_t{{2.2, -1.0}, {5.1}}, comp_t{{2.1, -1.1}, {4.7}});
  }
}
