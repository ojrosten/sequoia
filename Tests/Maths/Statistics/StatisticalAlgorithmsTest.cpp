////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StatisticalAlgorithmsTest.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"

#include <vector>

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view statistical_algorithms_test::source_file() const noexcept
  {
    return __FILE__;
  }

  void statistical_algorithms_test::run_tests()
  {
    using namespace sequoia::maths;

    std::vector<double> data{};

    //

    auto m{mean(data.begin(), data.end())};
    check(LINE(""), !m.has_value());

    auto sq{cummulative_square_diffs(data.begin(), data.end())};
    check(LINE(""), !sq.first.has_value());
    check(LINE(""), !sq.second.has_value());

    auto var{variance(data.begin(), data.end())};
    check(LINE(""), !var.first.has_value());
    check(LINE(""), !var.second.has_value());

    auto uvar{sample_variance(data.begin(), data.end())};
    check(LINE(""), !uvar.first.has_value());
    check(LINE(""), !uvar.second.has_value());

    auto sd{standard_deviation(data.begin(), data.end())};
    check(LINE(""), !sd.first.has_value());
    check(LINE(""), !sd.second.has_value());

    auto ssd{sample_standard_deviation(data.begin(), data.end())};
    check(LINE(""), !ssd.first.has_value());
    check(LINE(""), !ssd.second.has_value());

    // [2]
    data.push_back(2);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, LINE(""), m.value(), 2.0);

    check(equality, LINE(""), sq.first.value(), 0.0);
    check(equality, LINE(""), sq.second.value(), 2.0);

    check(equality, LINE(""), var.first.value(), 0.0);
    check(equality, LINE(""), var.second.value(), 2.0);

    check(LINE(""), !uvar.first.has_value());
    check(equality, LINE(""), uvar.second.value(), 2.0);

    check(equality, LINE(""), sd.first.value(), 0.0);
    check(equality, LINE(""), sd.second.value(), 2.0);

    check(LINE(""), !ssd.first.has_value());
    check(equality, LINE(""), ssd.second.value(), 2.0);

    // [2][4]
    data.push_back(4);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, LINE(""), m.value(), 3.0);

    check(equality, LINE(""), sq.first.value(), 2.0);
    check(equality, LINE(""), sq.second.value(), 3.0);

    check(equality, LINE(""), var.first.value(), 1.0);
    check(equality, LINE(""), var.second.value(), 3.0);

    check(equality, LINE(""), uvar.first.value(), 2.0);
    check(equality, LINE(""), uvar.second.value(), 3.0);

    check(equality, LINE(""), sd.first.value(), 1.0);
    check(equality, LINE(""), sd.second.value(), 3.0);

    check(equality, LINE(""), ssd.first.value(), 2.0);
    check(equality, LINE(""), ssd.second.value(), 3.0);

    // [2][4][9]
    data.push_back(9);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check(equality, LINE(""), m.value(), 5.0);

    check(equality, LINE(""), sq.first.value(), 26.0);
    check(equality, LINE(""), sq.second.value(), 5.0);

    check(equality, LINE(""), var.first.value(), 26.0/3);
    check(equality, LINE(""), var.second.value(), 5.0);

    check(equality, LINE(""), uvar.first.value(), 13.0);
    check(equality, LINE(""), uvar.second.value(), 5.0);

    check(equality, LINE(""), sd.first.value(), std::sqrt(26.0/3.0));
    check(equality, LINE(""), sd.second.value(), 5.0);

    check(equality, LINE(""), ssd.first.value(), std::sqrt(26.0/1.5));
    check(equality, LINE(""), ssd.second.value(), 5.0);
  }
}
