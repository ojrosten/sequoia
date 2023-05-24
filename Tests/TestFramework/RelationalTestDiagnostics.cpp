////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RelationalTestDiagnostics.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path relational_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void relational_false_positive_diagnostics::run_tests()
  {
    basic_tests();
    range_tests();
    container_tests();
  }

  void relational_false_positive_diagnostics::basic_tests()
  {
    check(within_tolerance{1.0}, report_line(""), 3.0, 5.0);
    check(within_tolerance{1.0}, report_line(""),
           7.0, 5.0,
           tutor{[](double, double){ return "Tweak your tolerance!"; }});

    check(std::ranges::less{},          report_line("<"),  5, 4);
    check(std::ranges::less_equal{},    report_line("<="), 5, 4);
    check(std::ranges::greater{},       report_line(">"),  4, 5);
    check(std::ranges::greater_equal{}, report_line(">="), 4, 5);
  }

  void relational_false_positive_diagnostics::range_tests()
  {
    {
      std::vector<double> v{0.5, 0.6}, p{-0.1, 1.0};
      check(within_tolerance{0.5}, report_line(""), v.cbegin(), v.cend(), p.cbegin(), p.cend());
      check(within_tolerance{0.5}, report_line(""),
            v.cbegin(), v.cend(), p.cbegin(), p.cend(),
            tutor{[](double, double) { return "Consider increasing tolerance!"; }});

      p = {0.5, 1.2};
      check(within_tolerance{0.5}, report_line(""), v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }

    {
      std::vector<int> v{4, 5}, p{5, 4};
      check(std::ranges::less{}, report_line("<"),v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }
  }

  void relational_false_positive_diagnostics::container_tests()
  {
    check(within_tolerance{0.5}, report_line(""), std::vector<double>{2.2, -1.0}, std::vector<double>{2.1, -1.8});

    using comp_t = std::vector<std::vector<double>>;
    check(within_tolerance{0.5}, report_line(""), comp_t{{2.2, -1.0}, {5.1}}, comp_t{{2.1, -1.1}, {3.7}});
  }

  [[nodiscard]]
  std::filesystem::path relational_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void relational_false_negative_diagnostics::run_tests()
  {
    basic_tests();
    range_tests();
    container_tests();
  }

  void relational_false_negative_diagnostics::basic_tests()
  {
    check(within_tolerance{1.0}, report_line(""), 4.5, 5.0);
    check(within_tolerance{1.0}, report_line(""), 5.5, 5.0);

    check(within_tolerance{0.5}, report_line(""), 4.5, 5.0);
    check(within_tolerance{0.5}, report_line(""), 5.5, 5.0);

    check(std::ranges::less{},          report_line("<"),  4, 5);
    check(std::ranges::less_equal{},    report_line("<="), 4, 5);
    check(std::ranges::greater{},       report_line(">"),  5, 4);
    check(std::ranges::greater_equal{}, report_line(">="), 5, 4);
  }

  void relational_false_negative_diagnostics::range_tests()
  {
    {
      std::vector<double> v{0.5, 0.6}, p{0, 1.0};
      check(within_tolerance{0.5}, report_line(""), v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }

    {
      std::vector<int> v{4, 3}, p{5, 4};
      check(std::ranges::less{}, report_line("<"), v.cbegin(), v.cend(), p.cbegin(), p.cend());
    }
  }

  void relational_false_negative_diagnostics::container_tests()
  {
    check(within_tolerance{0.5}, report_line(""), std::vector<double>{2.2, -1.0}, std::vector<double>{2.1, -1.1});

    using comp_t = std::vector<std::vector<double>>;
    check(within_tolerance{0.5}, report_line(""), comp_t{{2.2, -1.0}, {5.1}}, comp_t{{2.1, -1.1}, {4.7}});
  }
}
