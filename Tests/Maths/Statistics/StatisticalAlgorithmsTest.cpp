////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StatisticalAlgorithmsTest.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path statistical_algorithms_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void statistical_algorithms_test::run_tests()
  {
    using namespace sequoia::maths;

    std::vector<double> data{};

    //

    auto m{mean(data.begin(), data.end())};
    check(report_line(""), !m.has_value());

    auto sq{cummulative_square_diffs(data.begin(), data.end())};
    check(report_line(""), !sq.first.has_value());
    check(report_line(""), !sq.second.has_value());

    auto var{variance(data.begin(), data.end())};
    check(report_line(""), !var.first.has_value());
    check(report_line(""), !var.second.has_value());

    auto uvar{sample_variance(data.begin(), data.end())};
    check(report_line(""), !uvar.first.has_value());
    check(report_line(""), !uvar.second.has_value());

    auto sd{standard_deviation(data.begin(), data.end())};
    check(report_line(""), !sd.first.has_value());
    check(report_line(""), !sd.second.has_value());

    auto ssd{sample_standard_deviation(data.begin(), data.end())};
    check(report_line(""), !ssd.first.has_value());
    check(report_line(""), !ssd.second.has_value());

    // [2]
    data.push_back(2);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, report_line(""), m.value(), 2.0);

    check(equality, report_line(""), sq.first.value(), 0.0);
    check(equality, report_line(""), sq.second.value(), 2.0);

    check(equality, report_line(""), var.first.value(), 0.0);
    check(equality, report_line(""), var.second.value(), 2.0);

    check(report_line(""), !uvar.first.has_value());
    check(equality, report_line(""), uvar.second.value(), 2.0);

    check(equality, report_line(""), sd.first.value(), 0.0);
    check(equality, report_line(""), sd.second.value(), 2.0);

    check(report_line(""), !ssd.first.has_value());
    check(equality, report_line(""), ssd.second.value(), 2.0);

    // [2][4]
    data.push_back(4);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, report_line(""), m.value(), 3.0);

    check(equality, report_line(""), sq.first.value(), 2.0);
    check(equality, report_line(""), sq.second.value(), 3.0);

    check(equality, report_line(""), var.first.value(), 1.0);
    check(equality, report_line(""), var.second.value(), 3.0);

    check(equality, report_line(""), uvar.first.value(), 2.0);
    check(equality, report_line(""), uvar.second.value(), 3.0);

    check(equality, report_line(""), sd.first.value(), 1.0);
    check(equality, report_line(""), sd.second.value(), 3.0);

    check(equality, report_line(""), ssd.first.value(), 2.0);
    check(equality, report_line(""), ssd.second.value(), 3.0);

    // [2][4][9]
    data.push_back(9);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, report_line(""), m.value(), 5.0);

    check(equality, report_line(""), sq.first.value(), 26.0);
    check(equality, report_line(""), sq.second.value(), 5.0);

    check(equality, report_line(""), var.first.value(), 26.0/3);
    check(equality, report_line(""), var.second.value(), 5.0);

    check(equality, report_line(""), uvar.first.value(), 13.0);
    check(equality, report_line(""), uvar.second.value(), 5.0);

    check(equality, report_line(""), sd.first.value(), std::sqrt(26.0/3.0));
    check(equality, report_line(""), sd.second.value(), 5.0);

    check(equality, report_line(""), ssd.first.value(), std::sqrt(26.0/1.5));
    check(equality, report_line(""), ssd.second.value(), 5.0);
  }
}
