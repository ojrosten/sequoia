////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StatisticalAlgorithmsTest.hpp"

#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"
#include "sequoia/TestFramework/SumTypeCheckers.hpp"

#include <complex>
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
    template<class T, class Data>
    inline constexpr bool mean_accepts{
      requires { maths::mean<T>(std::declval<Data>()); }
    };

    template<class T, class Data>
    inline constexpr bool cumulative_square_diffs_accepts{
      requires { maths::cumulative_square_diffs<T>(std::declval<Data>()); }
    };

    template<class T, class Data>
    inline constexpr bool variance_accepts{
      requires { maths::variance<T>(std::declval<Data>()); }
    };

    template<class T, class Data>
    inline constexpr bool sample_variance_accepts{
      requires { maths::sample_variance<T>(std::declval<Data>()); }
    };

    template<class T, class Data>
    inline constexpr bool standard_deviation_accepts{
      requires { maths::standard_deviation<T>(std::declval<Data>()); }
    };

    template<class T, class Data>
    inline constexpr bool sample_standard_deviation_accepts{
      requires { maths::sample_standard_deviation<T>(std::declval<Data>()); }
    };

    template<class T, class Data>
    inline constexpr bool winsorized_sample_variance_accepts{
      requires { maths::winsorized_sample_variance<T>(std::declval<Data>(), 0); }
    };

    template<class T, class Data, class Estimator>
    inline constexpr bool sample_standard_deviation_accepts_estimator{
      requires { maths::sample_standard_deviation<T>(std::declval<Data>(), std::declval<Estimator>()); }
    };

    /// Returns a fixed estimate, so that a test can tell that it was called
    struct fixed_estimator
    {
      template<class T = void, maths::statistics_expressible_in<T> Data>
      [[nodiscard]]
      maths::standard_deviation_and_mean<maths::statistic_value_type_t<T, Data>> operator()(Data&&) const
      {
        return {.standard_deviation{42}};
      }
    };

    /// Its first template parameter is the data's type, not the type requested;
    /// it is only probed, so it needs no definition
    struct estimator_without_requested_type
    {
      template<class Data>
      [[nodiscard]]
      [[maybe_unused]]
      maths::standard_deviation_and_mean<double> operator()(Data&&) const;
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

    // Both directions, since a constraint rejecting everything would pass the
    // refusals on its own
    using single_pass = std::ranges::istream_view<double>;

    STATIC_CHECK(( std::ranges::input_range<single_pass> && !std::ranges::forward_range<single_pass>));
    STATIC_CHECK(( std::ranges::forward_range<forward_only_view>
                && !std::ranges::bidirectional_range<forward_only_view>
                && !std::ranges::range<const forward_only_view>));

    STATIC_CHECK(!mean_accepts<void, single_pass>);
    STATIC_CHECK(!cumulative_square_diffs_accepts<void, single_pass>);
    STATIC_CHECK(!variance_accepts<void, single_pass>);
    STATIC_CHECK(!sample_variance_accepts<void, single_pass>);
    STATIC_CHECK(!standard_deviation_accepts<void, single_pass>);
    STATIC_CHECK(!sample_standard_deviation_accepts<void, single_pass>);
    STATIC_CHECK(!winsorized_sample_variance_accepts<void, single_pass>);

    STATIC_CHECK(mean_accepts<void, forward_only_view>);
    STATIC_CHECK(cumulative_square_diffs_accepts<void, forward_only_view>);
    STATIC_CHECK(variance_accepts<void, forward_only_view>);
    STATIC_CHECK(sample_variance_accepts<void, forward_only_view>);
    STATIC_CHECK(standard_deviation_accepts<void, forward_only_view>);
    STATIC_CHECK(sample_standard_deviation_accepts<void, forward_only_view>);
    STATIC_CHECK(winsorized_sample_variance_accepts<void, forward_only_view>);

    std::vector<double> data{};

    //

    auto m{mean(data)};
    check("", !m.has_value());

    auto sq{cumulative_square_diffs(data)};
    check("", !sq.sum_of_square_diffs.has_value());
    check("", !sq.mean.has_value());

    auto var{variance(data)};
    check("", !var.variance.has_value());
    check("", !var.mean.has_value());

    auto uvar{sample_variance(data)};
    check("", !uvar.variance.has_value());
    check("", !uvar.mean.has_value());

    auto sd{standard_deviation(data)};
    check("", !sd.standard_deviation.has_value());
    check("", !sd.mean.has_value());

    auto ssd{sample_standard_deviation(data)};
    check("", !ssd.standard_deviation.has_value());
    check("", !ssd.mean.has_value());

    // [2]
    data.push_back(2);

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

    check(equality, "", m.value(),                      2.0);

    check(equality, "", sq.sum_of_square_diffs.value(), 0.0);
    check(equality, "", sq.mean.value(),                2.0);

    check(equality, "", var.variance.value(),           0.0);
    check(equality, "", var.mean.value(),               2.0);

    check("", !uvar.variance.has_value());
    check(equality, "", uvar.mean.value(),              2.0);

    check(equality, "", sd.standard_deviation.value(),  0.0);
    check(equality, "", sd.mean.value(),                2.0);

    check("", !ssd.standard_deviation.has_value());
    check(equality, "", ssd.mean.value(),               2.0);

    // [2][4]
    data.push_back(4);

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

    check(equality, "", m.value(),                      3.0);

    check(equality, "", sq.sum_of_square_diffs.value(), 2.0);
    check(equality, "", sq.mean.value(),                3.0);

    check(equality, "", var.variance.value(),           1.0);
    check(equality, "", var.mean.value(),               3.0);

    check(equality, "", uvar.variance.value(),          2.0);
    check(equality, "", uvar.mean.value(),              3.0);

    check(equality, "", sd.standard_deviation.value(),  1.0);
    check(equality, "", sd.mean.value(),                3.0);

    check(equality, "", ssd.standard_deviation.value(), 2.0);
    check(equality, "", ssd.mean.value(),               3.0);

    // [2][4][9]
    data.push_back(9);

    m = mean(data);
    sq = cumulative_square_diffs(data);
    var = variance(data);
    uvar = sample_variance(data);
    sd = standard_deviation(data);
    ssd = sample_standard_deviation(data);

    check(equality, "", m.value(),                      5.0);

    check(equality, "", sq.sum_of_square_diffs.value(), 26.0);
    check(equality, "", sq.mean.value(),                5.0);

    check(equality, "", var.variance.value(),           26.0/3);
    check(equality, "", var.mean.value(),               5.0);

    check(equality, "", uvar.variance.value(),          13.0);
    check(equality, "", uvar.mean.value(),              5.0);

    check(equality, "", sd.standard_deviation.value(),  std::sqrt(26.0/3.0));
    check(equality, "", sd.mean.value(),                5.0);

    check(equality, "", ssd.standard_deviation.value(), std::sqrt(26.0/1.5));
    check(equality, "", ssd.mean.value(),               5.0);

    test_winsorized_sample_variance();
    test_forward_only_view();
    test_types_admitted();
    test_narrower_type_requested();
    test_wider_type_requested();
    test_custom_estimator();
  }

  void statistical_algorithms_test::test_winsorized_sample_variance()
  {
    using namespace sequoia::maths;

    auto winsorized{
      [](const std::vector<double>& data, std::size_t numReplacedAtEachEnd) {
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
      const auto [var, mean]{winsorized({2, 4, 9}, std::numeric_limits<std::size_t>::max() / 2 + 1)};
      check("So many replaced that twice the number wraps to zero: no variance", !var.has_value());
      check("So many replaced that twice the number wraps to zero: no mean", !mean.has_value());
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

    check(equality, "Mean",                                m.value(),                      5.0);
    check(equality, "Cumulative square diffs",             sq.sum_of_square_diffs.value(), 26.0);
    check(equality, "Cumulative square diffs: the mean",   sq.mean.value(),                5.0);
    check(equality, "Variance",                            var.variance.value(),           26.0/3);
    check(equality, "Variance: the mean",                  var.mean.value(),               5.0);
    check(equality, "Sample variance",                     uvar.variance.value(),          13.0);
    check(equality, "Sample variance: the mean",           uvar.mean.value(),              5.0);
    check(equality, "Standard deviation",                  sd.standard_deviation.value(),  std::sqrt(26.0/3.0));
    check(equality, "Standard deviation: the mean",        sd.mean.value(),                5.0);
    check(equality, "Sample standard deviation",           ssd.standard_deviation.value(), std::sqrt(26.0/1.5));
    check(equality, "Sample standard deviation: the mean", ssd.mean.value(),               5.0);
    check(equality, "Winsorized variance",                 wsv.variance.value(),           0.0);
    check(equality, "Winsorized variance: the mean",       wsv.mean.value(),               4.0);
  }

  void statistical_algorithms_test::test_types_admitted()
  {
    using ints = const std::vector<int>&;

    STATIC_CHECK(!mean_accepts<void, ints>);
    STATIC_CHECK(!cumulative_square_diffs_accepts<void, ints>);
    STATIC_CHECK(!variance_accepts<void, ints>);
    STATIC_CHECK(!sample_variance_accepts<void, ints>);
    STATIC_CHECK(!standard_deviation_accepts<void, ints>);
    STATIC_CHECK(!sample_standard_deviation_accepts<void, ints>);
    STATIC_CHECK(!winsorized_sample_variance_accepts<void, ints>);

    STATIC_CHECK(mean_accepts<double, ints>);
    STATIC_CHECK(cumulative_square_diffs_accepts<double, ints>);
    STATIC_CHECK(variance_accepts<double, ints>);
    STATIC_CHECK(sample_variance_accepts<double, ints>);
    STATIC_CHECK(standard_deviation_accepts<double, ints>);
    STATIC_CHECK(sample_standard_deviation_accepts<double, ints>);
    STATIC_CHECK(winsorized_sample_variance_accepts<double, ints>);

    STATIC_CHECK(!mean_accepts<int,          const std::vector<double>&>);
    STATIC_CHECK(!mean_accepts<const double, const std::vector<double>&>);
    STATIC_CHECK(!mean_accepts<double,       const std::vector<std::complex<double>>&>);

    check(equality, "The mean of integers, as a double", maths::mean<double>(std::vector<int>{1, 2}).value(), 1.5);
  }

  void statistical_algorithms_test::test_narrower_type_requested()
  {
    // These data are exact in double, but float's spacing near 1e8 is 8. The
    // sums of squared deviations, 37542262 and, winsorized, 37531630, exceed
    // 2^24, so float rounds them too, and each division and square root
    // rounds differently in float.
    const std::vector<double> data{1e8, 1e8 + 1, 1e8 + 11, 1e8 + 29, 1e8 + 5316, 1e8 + 5317};

    auto narrowed{
      [](const std::optional<double>& x) { return x.transform([](double value) { return static_cast<float>(value); }); }
    };

    const auto m   {maths::mean(data)};
    const auto sq  {maths::cumulative_square_diffs(data)};
    const auto var {maths::variance(data)};
    const auto uvar{maths::sample_variance(data)};
    const auto sd  {maths::standard_deviation(data)};
    const auto ssd {maths::sample_standard_deviation(data)};
    const auto wsv {maths::winsorized_sample_variance(data, 1)};

    const auto fm   {maths::mean<float>(data)};
    const auto fsq  {maths::cumulative_square_diffs<float>(data)};
    const auto fvar {maths::variance<float>(data)};
    const auto fuvar{maths::sample_variance<float>(data)};
    const auto fsd  {maths::standard_deviation<float>(data)};
    const auto fssd {maths::sample_standard_deviation<float>(data)};
    const auto fwsv {maths::winsorized_sample_variance<float>(data, 1)};

    check(equality, "Mean",                                fm,                      narrowed(m));
    check(equality, "Cumulative square diffs",             fsq.sum_of_square_diffs, narrowed(sq.sum_of_square_diffs));
    check(equality, "Cumulative square diffs: the mean",   fsq.mean,                narrowed(sq.mean));
    check(equality, "Variance",                            fvar.variance,           narrowed(var.variance));
    check(equality, "Variance: the mean",                  fvar.mean,               narrowed(var.mean));
    check(equality, "Sample variance",                     fuvar.variance,          narrowed(uvar.variance));
    check(equality, "Sample variance: the mean",           fuvar.mean,              narrowed(uvar.mean));
    check(equality, "Standard deviation",                  fsd.standard_deviation,  narrowed(sd.standard_deviation));
    check(equality, "Standard deviation: the mean",        fsd.mean,                narrowed(sd.mean));
    check(equality, "Sample standard deviation",           fssd.standard_deviation, narrowed(ssd.standard_deviation));
    check(equality, "Sample standard deviation: the mean", fssd.mean,               narrowed(ssd.mean));
    check(equality, "Winsorized variance",                 fwsv.variance,           narrowed(wsv.variance));
    check(equality, "Winsorized variance: the mean",       fwsv.mean,               narrowed(wsv.mean));
  }

  void statistical_algorithms_test::test_wider_type_requested()
  {
    // At and beyond 2^25 in magnitude, adding -1 to a float changes nothing,
    // so a float accumulator loses both -1s, with or without winsorizing.
    const std::vector<float> data{-67108864.0f, -33554432.0f, -1.0f, -1.0f, 33554436.0f, 67108864.0f};
    const std::vector<double> widened(data.begin(), data.end());

    const auto m   {maths::mean(widened)};
    const auto sq  {maths::cumulative_square_diffs(widened)};
    const auto var {maths::variance(widened)};
    const auto uvar{maths::sample_variance(widened)};
    const auto sd  {maths::standard_deviation(widened)};
    const auto ssd {maths::sample_standard_deviation(widened)};
    const auto wsv {maths::winsorized_sample_variance(widened, 1)};

    const auto dm   {maths::mean<double>(data)};
    const auto dsq  {maths::cumulative_square_diffs<double>(data)};
    const auto dvar {maths::variance<double>(data)};
    const auto duvar{maths::sample_variance<double>(data)};
    const auto dsd  {maths::standard_deviation<double>(data)};
    const auto dssd {maths::sample_standard_deviation<double>(data)};
    const auto dwsv {maths::winsorized_sample_variance<double>(data, 1)};

    check(equality, "Mean",                                dm,                      m);
    check(equality, "Cumulative square diffs",             dsq.sum_of_square_diffs, sq.sum_of_square_diffs);
    check(equality, "Cumulative square diffs: the mean",   dsq.mean,                sq.mean);
    check(equality, "Variance",                            dvar.variance,           var.variance);
    check(equality, "Variance: the mean",                  dvar.mean,               var.mean);
    check(equality, "Sample variance",                     duvar.variance,          uvar.variance);
    check(equality, "Sample variance: the mean",           duvar.mean,              uvar.mean);
    check(equality, "Standard deviation",                  dsd.standard_deviation,  sd.standard_deviation);
    check(equality, "Standard deviation: the mean",        dsd.mean,                sd.mean);
    check(equality, "Sample standard deviation",           dssd.standard_deviation, ssd.standard_deviation);
    check(equality, "Sample standard deviation: the mean", dssd.mean,               ssd.mean);
    check(equality, "Winsorized variance",                 dwsv.variance,           wsv.variance);
    check(equality, "Winsorized variance: the mean",       dwsv.mean,               wsv.mean);
  }

  void statistical_algorithms_test::test_custom_estimator()
  {
    using doubles = const std::vector<double>&;

    STATIC_CHECK(sample_standard_deviation_accepts_estimator<float, doubles, fixed_estimator>);
    STATIC_CHECK(!sample_standard_deviation_accepts_estimator<void, doubles, estimator_without_requested_type>);

    const auto ssd{maths::sample_standard_deviation<float>(std::vector<double>{2, 4, 9}, fixed_estimator{})};
    check(equality, "The custom estimator's estimate", ssd.standard_deviation, std::optional<float>{42});
  }
}
