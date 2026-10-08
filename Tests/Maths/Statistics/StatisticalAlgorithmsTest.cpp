////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StatisticalAlgorithmsTest.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"

#include <iterator>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    // Parameterized so that an unsatisfiable requirement is a substitution failure rather than an ill-formed program
    template<class Iter> inline constexpr bool mean_accepts                      {requires(Iter i) { maths::mean(i, i); }};
    template<class Iter> inline constexpr bool cumulative_square_diffs_accepts   {requires(Iter i) { maths::cumulative_square_diffs(i, i); }};
    template<class Iter> inline constexpr bool variance_accepts                  {requires(Iter i) { maths::variance(i, i); }};
    template<class Iter> inline constexpr bool sample_variance_accepts           {requires(Iter i) { maths::sample_variance(i, i); }};
    template<class Iter> inline constexpr bool standard_deviation_accepts        {requires(Iter i) { maths::standard_deviation(i, i); }};
    template<class Iter> inline constexpr bool sample_standard_deviation_accepts {requires(Iter i) { maths::sample_standard_deviation(i, i); }};
    template<class Iter> inline constexpr bool winsorized_sample_variance_accepts{requires(Iter i) { maths::winsorized_sample_variance(i, i, 0); }};
  }

  [[nodiscard]]
  std::filesystem::path statistical_algorithms_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void statistical_algorithms_test::run_tests()
  {
    using namespace sequoia::maths;

    // Both directions, since a constraint rejecting everything would pass the refusals on its own
    using single_pass = std::istream_iterator<double>;
    using multi_pass  = std::vector<double>::const_iterator;

    STATIC_CHECK(( std::input_iterator<single_pass> && !std::forward_iterator<single_pass>));
    STATIC_CHECK(( std::forward_iterator<multi_pass>));

    STATIC_CHECK(!mean_accepts<single_pass>);
    STATIC_CHECK(!cumulative_square_diffs_accepts<single_pass>);
    STATIC_CHECK(!variance_accepts<single_pass>);
    STATIC_CHECK(!sample_variance_accepts<single_pass>);
    STATIC_CHECK(!standard_deviation_accepts<single_pass>);
    STATIC_CHECK(!sample_standard_deviation_accepts<single_pass>);
    STATIC_CHECK(!winsorized_sample_variance_accepts<single_pass>);

    STATIC_CHECK(mean_accepts<multi_pass>);
    STATIC_CHECK(cumulative_square_diffs_accepts<multi_pass>);
    STATIC_CHECK(variance_accepts<multi_pass>);
    STATIC_CHECK(sample_variance_accepts<multi_pass>);
    STATIC_CHECK(standard_deviation_accepts<multi_pass>);
    STATIC_CHECK(sample_standard_deviation_accepts<multi_pass>);
    STATIC_CHECK(winsorized_sample_variance_accepts<multi_pass>);

    std::vector<double> data{};

    //

    auto m{mean(data.begin(), data.end())};
    check("", !m.has_value());

    auto sq{cumulative_square_diffs(data.begin(), data.end())};
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
    sq = cumulative_square_diffs(data.begin(), data.end());
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
    sq = cumulative_square_diffs(data.begin(), data.end());
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
    sq = cumulative_square_diffs(data.begin(), data.end());
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

    test_winsorized_sample_variance();
  }

  void statistical_algorithms_test::test_winsorized_sample_variance()
  {
    using namespace sequoia::maths;

    auto winsorized{
      [](const std::vector<double>& data, std::ptrdiff_t numReplacedAtEachEnd) {
        return winsorized_sample_variance(data.cbegin(), data.cend(), numReplacedAtEachEnd);
      }
    };

    {
      const auto [var, mean]{winsorized({}, 0)};
      check("No data: no variance", !var.has_value());
      check("No data: no mean", !mean.has_value());
    }

    {
      const auto [var, mean]{winsorized({2}, 0)};
      check("A single datum: no variance", !var.has_value());
      check(equality, "A single datum: its mean", mean.value(), 2.0);
    }

    {
      const auto [var, mean]{winsorized({2, 4}, 1)};
      check("Every datum replaced: no variance", !var.has_value());
      check("Every datum replaced: no mean", !mean.has_value());
    }

    {
      const auto [var, mean]{winsorized({2, 4, 9}, -1)};
      check("A negative number replaced: no variance", !var.has_value());
      check("A negative number replaced: no mean", !mean.has_value());
    }

    {
      const auto [var, mean]{winsorized({2, 4, 9}, 0)};
      check(equality, "Nothing replaced: the sample variance", var.value(), 13.0);
      check(equality, "Nothing replaced: the mean", mean.value(), 5.0);
    }

    {
      const auto [var, mean]{winsorized({2, 4, 9}, 1)};
      check(equality, "A single datum remains: zero variance", var.value(), 0.0);
      check(equality, "A single datum remains: that datum", mean.value(), 4.0);
    }

    {
      // Winsorized to [0, 0, 1, 7, 7]. The middle three alone have a different
      // variance and mean.
      const auto [var, mean]{winsorized({-5, 0, 1, 7, 20}, 1)};
      check(equality, "One replaced at each end: the variance", var.value(), 13.5);
      check(equality, "One replaced at each end: the mean", mean.value(), 3.0);
    }
  }
}
