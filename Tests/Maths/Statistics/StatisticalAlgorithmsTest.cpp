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
    check("", !m.has_value());

    auto sq{cummulative_square_diffs(data.begin(), data.end())};
    check("", !sq.first.has_value());
    check("", !sq.second.has_value());

    auto var{variance(data.begin(), data.end())};
    check("", !var.first.has_value());
    check("", !var.second.has_value());

    auto uvar{sample_variance(data.begin(), data.end())};
    check("", !uvar.first.has_value());
    check("", !uvar.second.has_value());

    auto sd{standard_deviation(data.begin(), data.end())};
    check("", !sd.first.has_value());
    check("", !sd.second.has_value());

    auto ssd{sample_standard_deviation(data.begin(), data.end())};
    check("", !ssd.first.has_value());
    check("", !ssd.second.has_value());

    // [2]
    data.push_back(2);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, "", m.value(), 2.0);

    check(equality, "", sq.first.value(), 0.0);
    check(equality, "", sq.second.value(), 2.0);

    check(equality, "", var.first.value(), 0.0);
    check(equality, "", var.second.value(), 2.0);

    check("", !uvar.first.has_value());
    check(equality, "", uvar.second.value(), 2.0);

    check(equality, "", sd.first.value(), 0.0);
    check(equality, "", sd.second.value(), 2.0);

    check("", !ssd.first.has_value());
    check(equality, "", ssd.second.value(), 2.0);

    // [2][4]
    data.push_back(4);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, "", m.value(), 3.0);

    check(equality, "", sq.first.value(), 2.0);
    check(equality, "", sq.second.value(), 3.0);

    check(equality, "", var.first.value(), 1.0);
    check(equality, "", var.second.value(), 3.0);

    check(equality, "", uvar.first.value(), 2.0);
    check(equality, "", uvar.second.value(), 3.0);

    check(equality, "", sd.first.value(), 1.0);
    check(equality, "", sd.second.value(), 3.0);

    check(equality, "", ssd.first.value(), 2.0);
    check(equality, "", ssd.second.value(), 3.0);

    // [2][4][9]
    data.push_back(9);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, "", m.value(), 5.0);

    check(equality, "", sq.first.value(), 26.0);
    check(equality, "", sq.second.value(), 5.0);

    check(equality, "", var.first.value(), 26.0/3);
    check(equality, "", var.second.value(), 5.0);

    check(equality, "", uvar.first.value(), 13.0);
    check(equality, "", uvar.second.value(), 5.0);

    check(equality, "", sd.first.value(), std::sqrt(26.0/3.0));
    check(equality, "", sd.second.value(), 5.0);

    check(equality, "", ssd.first.value(), std::sqrt(26.0/1.5));
    check(equality, "", ssd.second.value(), 5.0);
  }
}
