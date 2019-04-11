////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StatisticalAlgorithmsTest.hpp"

#include "StatisticalAlgorithms.hpp"

namespace sequoia::unit_testing
{
  void statistical_algorithms_test::run_tests()
  {
    using namespace sequoia::maths;
    
    std::vector<double> data{};

    //
    
    auto m{mean(data.begin(), data.end())};
    check(!m.has_value(), LINE(""));

    auto sq{cummulative_square_diffs(data.begin(), data.end())};
    check(!sq.first.has_value(), LINE(""));
    check(!sq.second.has_value(), LINE(""));

    auto var{variance(data.begin(), data.end())};
    check(!var.first.has_value(), LINE(""));
    check(!var.second.has_value(), LINE(""));

    auto uvar{sample_variance(data.begin(), data.end())};
    check(!uvar.first.has_value(), LINE(""));
    check(!uvar.second.has_value(), LINE(""));

    auto sd{standard_deviation(data.begin(), data.end())};
    check(!sd.first.has_value(), LINE(""));
    check(!sd.second.has_value(), LINE(""));

    auto ssd{sample_standard_deviation(data.begin(), data.end())};
    check(!ssd.first.has_value(), LINE(""));
    check(!ssd.second.has_value(), LINE(""));

    // [2]    
    data.push_back(2);
    
    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());
    
    check_equality(m.value(), 2.0, LINE(""));
    
    check_equality(sq.first.value(), 0.0, LINE(""));
    check_equality(sq.second.value(), 2.0, LINE(""));

    check_equality(var.first.value(), 0.0, LINE(""));
    check_equality(var.second.value(), 2.0, LINE(""));

    check(!uvar.first.has_value(), LINE(""));
    check_equality(uvar.second.value(), 2.0, LINE(""));

    check_equality(sd.first.value(), 0.0, LINE(""));
    check_equality(sd.second.value(), 2.0, LINE(""));

    check(!ssd.first.has_value(), LINE(""));
    check_equality(ssd.second.value(), 2.0, LINE(""));
    
    // [2][4]   
    data.push_back(4);
    
    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());
    
    check_equality(m.value(), 3.0, LINE(""));
    
    check_equality(sq.first.value(), 2.0, LINE(""));
    check_equality(sq.second.value(), 3.0, LINE(""));

    check_equality(var.first.value(), 1.0, LINE(""));
    check_equality(var.second.value(), 3.0, LINE(""));

    check_equality(uvar.first.value(), 2.0, LINE(""));
    check_equality(uvar.second.value(), 3.0, LINE(""));

    check_equality(sd.first.value(), 1.0, LINE(""));
    check_equality(sd.second.value(), 3.0, LINE(""));

    check_equality(ssd.first.value(), 2.0, LINE(""));
    check_equality(ssd.second.value(), 3.0, LINE(""));

    // [2][4][9]   
    data.push_back(9);

    m = mean(data.begin(), data.end());
    sq = cummulative_square_diffs(data.begin(), data.end());
    var = variance(data.begin(), data.end());
    uvar = sample_variance(data.begin(), data.end());
    sd = standard_deviation(data.begin(), data.end());
    ssd = sample_standard_deviation(data.begin(), data.end());

    check_equality(m.value(), 5.0, LINE(""));

    check_equality(sq.first.value(), 26.0, LINE(""));
    check_equality(sq.second.value(), 5.0, LINE(""));

    check_equality(var.first.value(), 26.0/3, LINE(""));
    check_equality(var.second.value(), 5.0, LINE(""));

    check_equality(uvar.first.value(), 13.0, LINE(""));
    check_equality(uvar.second.value(), 5.0, LINE(""));

    check_equality(sd.first.value(), std::sqrt(26.0/3.0), LINE(""));
    check_equality(sd.second.value(), 5.0, LINE(""));

    check_equality(ssd.first.value(), std::sqrt(26.0/1.5), LINE(""));
    check_equality(ssd.second.value(), 5.0, LINE(""));
  }
}
