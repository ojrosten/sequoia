////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StatisticalAlgorithmsTest.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"

#include <forward_list>
#include <limits>
#include <ranges>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    // Each probe is parameterized so that an unsatisfiable requirement is a
    // substitution failure rather than an ill-formed program
    template<class Data>
    inline constexpr bool mean_accepts{
      requires(Data d) { maths::mean(d); }
    };

    template<class Data>
    inline constexpr bool cumulative_square_diffs_accepts{
      requires(Data d) { maths::cumulative_square_diffs(d); }
    };

    template<class Data>
    inline constexpr bool variance_accepts{
      requires(Data d) { maths::variance(d); }
    };

    template<class Data>
    inline constexpr bool sample_variance_accepts{
      requires(Data d) { maths::sample_variance(d); }
    };

    template<class Data>
    inline constexpr bool standard_deviation_accepts{
      requires(Data d) { maths::standard_deviation(d); }
    };

    template<class Data>
    inline constexpr bool sample_standard_deviation_accepts{
      requires(Data d) { maths::sample_standard_deviation(d); }
    };

    template<class Data>
    inline constexpr bool winsorized_sample_variance_accepts{
      requires(Data d) { maths::winsorized_sample_variance(d, 0); }
    };
  }

  [[nodiscard]]
  std::filesystem::path statistical_algorithms_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void statistical_algorithms_test::run_tests()
  {
    using namespace sequoia::maths;

    auto keepAll{[](double) { return true; }};

    // Both directions, since a constraint rejecting everything would pass the refusals on its own
    using single_pass = std::ranges::istream_view<double>;
    using multi_pass  = std::ranges::filter_view<std::ranges::ref_view<std::forward_list<double>>, decltype(keepAll)>;

    STATIC_CHECK(( std::ranges::input_range<single_pass> && !std::ranges::forward_range<single_pass>));
    STATIC_CHECK(( std::ranges::forward_range<multi_pass>
                && !std::ranges::bidirectional_range<multi_pass>
                && !std::ranges::range<const multi_pass>));

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

    auto m{mean(data)};
    check("", !m.has_value());

    auto sq{cumulative_square_diffs(data)};
    check("", !sq.first.has_value());
    check("", !sq.second.has_value());

    auto var{variance(data)};
    check("", !var.first.has_value());
    check("", !var.second.has_value());

    auto uvar{sample_variance(data)};
    check("", !uvar.first.has_value());
    check("", !uvar.second.has_value());

    auto sd{standard_deviation(data)};
    check("", !sd.first.has_value());
    check("", !sd.second.has_value());

    auto ssd{sample_standard_deviation(data)};
    check("", !ssd.first.has_value());
    check("", !ssd.second.has_value());

    // [2]
    data.push_back(2);

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

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

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

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

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

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
    test_forward_only_view();
  }

  void statistical_algorithms_test::test_winsorized_sample_variance()
  {
    using namespace sequoia::maths;

    auto winsorized{
      [](const std::vector<double>& data, std::ptrdiff_t numReplacedAtEachEnd) {
        return winsorized_sample_variance(data, numReplacedAtEachEnd);
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
      const auto [var, mean]{winsorized({2, 4, 9}, std::numeric_limits<std::ptrdiff_t>::max())};
      check("The most that can be replaced: no variance", !var.has_value());
      check("The most that can be replaced: no mean", !mean.has_value());
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

  void statistical_algorithms_test::test_forward_only_view()
  {
    using namespace sequoia::maths;

    const std::forward_list<double> data{2, 4, 9};
    auto view{data | std::views::filter([](double) { return true; })};
    using view_type = decltype(view);

    STATIC_CHECK(( std::ranges::forward_range<view_type>
                && !std::ranges::bidirectional_range<view_type>
                && !std::ranges::range<const view_type>));

    check(equality, "Mean",                       mean(view).value(),                                5.0);
    check(equality, "Cumulative square diffs",    cumulative_square_diffs(view).first.value(),       26.0);
    check(equality, "Variance",                   variance(view).first.value(),                      26.0/3);
    check(equality, "Sample variance",            sample_variance(view).first.value(),               13.0);
    check(equality, "Standard deviation",         standard_deviation(view).first.value(),            std::sqrt(26.0/3));
    check(equality, "Sample standard deviation",  sample_standard_deviation(view).first.value(),     std::sqrt(52.0/3));
    check(equality, "Winsorized sample variance", winsorized_sample_variance(view, 0).first.value(), 13.0);
  }
}
