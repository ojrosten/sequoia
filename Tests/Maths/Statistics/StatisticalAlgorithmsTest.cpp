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
#include <utility>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    // Argument-dependent lookup on forward_only_view finds these overloads.
    // An unqualified call of mean, cumulative_square_diffs or variance within
    // the algorithms would therefore be ambiguous.
    namespace rival_overloads
    {
      struct keep_all
      {
        [[nodiscard]]
        constexpr bool operator()(double) const noexcept { return true; }
      };

      template<class Data> void mean(Data&&) = delete;
      template<class Data> void cumulative_square_diffs(Data&&) = delete;
      template<class Data> void variance(Data&&) = delete;
    }

    using forward_only_view
      = std::ranges::filter_view<std::ranges::ref_view<const std::forward_list<double>>, rival_overloads::keep_all>;

    // Each probe is parameterized so that an unsatisfiable requirement is a
    // substitution failure rather than an ill-formed program. Each passes an
    // rvalue, which a parameter of type `Data&` would refuse.
    template<class Data>
    inline constexpr bool mean_accepts{
      requires { maths::mean(std::declval<Data>()); }
    };

    template<class Data>
    inline constexpr bool cumulative_square_diffs_accepts{
      requires { maths::cumulative_square_diffs(std::declval<Data>()); }
    };

    template<class Data>
    inline constexpr bool variance_accepts{
      requires { maths::variance(std::declval<Data>()); }
    };

    template<class Data>
    inline constexpr bool sample_variance_accepts{
      requires { maths::sample_variance(std::declval<Data>()); }
    };

    template<class Data>
    inline constexpr bool standard_deviation_accepts{
      requires { maths::standard_deviation(std::declval<Data>()); }
    };

    template<class Data>
    inline constexpr bool sample_standard_deviation_accepts{
      requires { maths::sample_standard_deviation(std::declval<Data>()); }
    };

    template<class Data>
    inline constexpr bool winsorized_sample_variance_accepts{
      requires { maths::winsorized_sample_variance(std::declval<Data>(), 0); }
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

    // Both directions, since a constraint rejecting everything would pass the refusals on its own
    using single_pass = std::ranges::istream_view<double>;

    STATIC_CHECK(( std::ranges::input_range<single_pass> && !std::ranges::forward_range<single_pass>));
    STATIC_CHECK(( std::ranges::forward_range<forward_only_view>
                && !std::ranges::bidirectional_range<forward_only_view>
                && !std::ranges::range<const forward_only_view>));

    STATIC_CHECK(!mean_accepts<single_pass>);
    STATIC_CHECK(!cumulative_square_diffs_accepts<single_pass>);
    STATIC_CHECK(!variance_accepts<single_pass>);
    STATIC_CHECK(!sample_variance_accepts<single_pass>);
    STATIC_CHECK(!standard_deviation_accepts<single_pass>);
    STATIC_CHECK(!sample_standard_deviation_accepts<single_pass>);
    STATIC_CHECK(!winsorized_sample_variance_accepts<single_pass>);

    STATIC_CHECK(mean_accepts<forward_only_view>);
    STATIC_CHECK(cumulative_square_diffs_accepts<forward_only_view>);
    STATIC_CHECK(variance_accepts<forward_only_view>);
    STATIC_CHECK(sample_variance_accepts<forward_only_view>);
    STATIC_CHECK(standard_deviation_accepts<forward_only_view>);
    STATIC_CHECK(sample_standard_deviation_accepts<forward_only_view>);
    STATIC_CHECK(winsorized_sample_variance_accepts<forward_only_view>);

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

    check(equality, "",                                                   m.value(),           2.0);

    check(equality, "",                                                   sq.first.value(),    0.0);
    check(equality, "",                                                   sq.second.value(),   2.0);

    check(equality, "",                                                   var.first.value(),   0.0);
    check(equality, "",                                                   var.second.value(),  2.0);

    check("", !uvar.first.has_value());
    check(equality, "",                                                   uvar.second.value(), 2.0);

    check(equality, "",                                                   sd.first.value(),    0.0);
    check(equality, "",                                                   sd.second.value(),   2.0);

    check("", !ssd.first.has_value());
    check(equality, "",                                                   ssd.second.value(),  2.0);

    // [2][4]
    data.push_back(4);

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

    check(equality, "",                                                   m.value(),           3.0);

    check(equality, "",                                                   sq.first.value(),    2.0);
    check(equality, "",                                                   sq.second.value(),   3.0);

    check(equality, "",                                                   var.first.value(),   1.0);
    check(equality, "",                                                   var.second.value(),  3.0);

    check(equality, "",                                                   uvar.first.value(),  2.0);
    check(equality, "",                                                   uvar.second.value(), 3.0);

    check(equality, "",                                                   sd.first.value(),    1.0);
    check(equality, "",                                                   sd.second.value(),   3.0);

    check(equality, "",                                                   ssd.first.value(),   2.0);
    check(equality, "",                                                   ssd.second.value(),  3.0);

    // [2][4][9]
    data.push_back(9);

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

    check(equality, "",                                                   m.value(),           5.0);

    check(equality, "",                                                   sq.first.value(),    26.0);
    check(equality, "",                                                   sq.second.value(),   5.0);

    check(equality, "",                                                   var.first.value(),   26.0/3);
    check(equality, "",                                                   var.second.value(),  5.0);

    check(equality, "",                                                   uvar.first.value(),  13.0);
    check(equality, "",                                                   uvar.second.value(), 5.0);

    check(equality, "",                                                   sd.first.value(),    std::sqrt(26.0/3.0));
    check(equality, "",                                                   sd.second.value(),   5.0);

    check(equality, "",                                                   ssd.first.value(),   std::sqrt(26.0/1.5));
    check(equality, "",                                                   ssd.second.value(),  5.0);

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
    const std::forward_list<double> data{2, 4, 9};
    forward_only_view view{std::views::all(data), {}};

    const auto m   {maths::mean(view)};
    const auto sq  {maths::cumulative_square_diffs(view)};
    const auto var {maths::variance(view)};
    const auto uvar{maths::sample_variance(view)};
    const auto sd  {maths::standard_deviation(view)};
    const auto ssd {maths::sample_standard_deviation(view)};
    const auto wsv {maths::winsorized_sample_variance(view, 1)};

    check(equality, "Mean",                                               m.value(),           5.0);
    check(equality, "Cumulative square diffs",                            sq.first.value(),    26.0);
    check(equality, "Cumulative square diffs: the mean",                  sq.second.value(),   5.0);
    check(equality, "Variance",                                           var.first.value(),   26.0/3);
    check(equality, "Variance: the mean",                                 var.second.value(),  5.0);
    check(equality, "Sample variance",                                    uvar.first.value(),  13.0);
    check(equality, "Sample variance: the mean",                          uvar.second.value(), 5.0);
    check(equality, "Standard deviation",                                 sd.first.value(),    std::sqrt(26.0/3.0));
    check(equality, "Standard deviation: the mean",                       sd.second.value(),   5.0);
    check(equality, "Sample standard deviation",                          ssd.first.value(),   std::sqrt(26.0/1.5));
    check(equality, "Sample standard deviation: the mean",                ssd.second.value(),  5.0);
    check(equality, "Winsorized, one replaced at each end: the variance", wsv.first.value(),   0.0);
    check(equality, "Winsorized, one replaced at each end: the mean",     wsv.second.value(),  4.0);
  }
}
