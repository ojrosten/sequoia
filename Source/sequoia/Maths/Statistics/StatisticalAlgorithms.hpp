////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file StatisticalAlgorithms.hpp
    \brief Tools for statistical analysis.

    Each statistic of `data` is returned as `statistic_value_type_t<T, Data>`.
    It is computed in the common type of the type returned and the value type
    of the data, and converted once, on return. A statistic too large for the
    type returned is converted as the platform converts floating-point values.
    That typically gives an infinity; for a type with no infinity, the
    behaviour is undefined.
*/

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <optional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace sequoia::maths
{
  /** \brief `T`, or the value type of `Data` if `T` is `void`. */
  template<class T, std::ranges::forward_range Data>
    requires std::same_as<T, std::remove_cvref_t<T>>
  using statistic_value_type_t = std::conditional_t<std::is_void_v<T>, std::ranges::range_value_t<Data>, T>;

  namespace impl
  {
    template<class T, std::ranges::forward_range Data>
    using statistic_working_type_t
      = std::common_type_t<statistic_value_type_t<T, Data>, std::ranges::range_value_t<Data>>;
  }

  /** \brief The statistics of the forward range `Data` can be returned in the
             floating-point type that `T` requests, and computed in a
             floating-point type.
   */
  template<class Data, class T>
  concept statistics_expressible_in
    =    std::ranges::forward_range<Data>
      && std::floating_point<statistic_value_type_t<T, Data>>
      && std::floating_point<impl::statistic_working_type_t<T, Data>>;

  /** \brief The sum of the squared deviations of data from their mean, and the
             mean.
   */
  template<class T>
  struct sum_of_square_diffs_and_mean
  {
    std::optional<T> sum_of_square_diffs{}, mean{};
  };

  /** \brief A variance of data, and their mean. */
  template<class T>
  struct variance_and_mean
  {
    std::optional<T> variance{}, mean{};
  };

  /** \brief A standard deviation of data, and their mean. */
  template<class T>
  struct standard_deviation_and_mean
  {
    std::optional<T> standard_deviation{}, mean{};
  };

  /** \brief Returns the mean of `data`.

      There is no mean if `data` is empty.
   */
  template<class T = void, statistics_expressible_in<T> Data>
  [[nodiscard]]
  std::optional<statistic_value_type_t<T, Data>> mean(Data&& data)
  {
    using statistic_type = statistic_value_type_t<T, Data>;
    using working_type   = impl::statistic_working_type_t<T, Data>;

    if(const auto dist{std::ranges::distance(data)})
    {
      return static_cast<statistic_type>(
        std::ranges::fold_left(data, working_type{}, std::plus<>{}) / static_cast<working_type>(dist)
      );
    }

    return {};
  }

  /** \brief Returns the sum of the squared deviations of `data` from its mean,
             and the mean.

      \returns
      -# Neither, if `data` is empty;
      -# Both, otherwise.
   */
  template<class T = void, statistics_expressible_in<T> Data>
  [[nodiscard]]
  sum_of_square_diffs_and_mean<statistic_value_type_t<T, Data>> cumulative_square_diffs(Data&& data)
  {
    using statistic_type = statistic_value_type_t<T, Data>;
    using working_type   = impl::statistic_working_type_t<T, Data>;

    if(std::ranges::distance(data))
    {
      const working_type m{maths::mean<working_type>(data).value()};

      auto addSquareDiff{
        [m](working_type sum, working_type datum) { return sum + (datum - m)*(datum - m); }
      };

      const working_type sumOfSquareDiffs{std::ranges::fold_left(data, working_type{}, addSquareDiff)};

      return {
        .sum_of_square_diffs{static_cast<statistic_type>(sumOfSquareDiffs)},
        .mean{static_cast<statistic_type>(m)}
      };
    }

    return {};
  }

  /** \brief Returns the population variance and the mean of `data`.

      \returns
      -# Neither, if `data` is empty;
      -# Both, otherwise.
   */
  template<class T = void, statistics_expressible_in<T> Data>
  [[nodiscard]]
  variance_and_mean<statistic_value_type_t<T, Data>> variance(Data&& data)
  {
    using statistic_type = statistic_value_type_t<T, Data>;
    using working_type   = impl::statistic_working_type_t<T, Data>;

    if(const auto dist{std::ranges::distance(data)})
    {
      const auto [sq, mean]{maths::cumulative_square_diffs<working_type>(data)};

      return {
        .variance{static_cast<statistic_type>(sq.value() / static_cast<working_type>(dist))},
        .mean{static_cast<statistic_type>(mean.value())}
      };
    }

    return {};
  }

  /** \brief Returns the sample variance and the mean of `data`.

      \returns
      -# Neither, if `data` is empty;
      -# Only the mean, if there is a single datum;
      -# Both, otherwise.
   */
  template<class T = void, statistics_expressible_in<T> Data>
  [[nodiscard]]
  variance_and_mean<statistic_value_type_t<T, Data>> sample_variance(Data&& data)
  {
    using statistic_type = statistic_value_type_t<T, Data>;
    using working_type   = impl::statistic_working_type_t<T, Data>;

    if(const auto dist{std::ranges::distance(data)}; !dist)
    {
      return {};
    }
    else if(dist == 1)
    {
      return {.mean{maths::mean<T>(data)}};
    }
    else
    {
      const auto [sq, mean]{maths::cumulative_square_diffs<working_type>(data)};

      return {
        .variance{static_cast<statistic_type>(sq.value() / static_cast<working_type>(dist - 1))},
        .mean{static_cast<statistic_type>(mean.value())}
      };
    }
  }

  /** \brief Returns the sample variance and the mean of `data` once the
             first and last `numReplacedAtEachEnd` are winsorized.

      Winsorizing replaces each of the first `numReplacedAtEachEnd` data with
      the first datum after them, and each of the last `numReplacedAtEachEnd`
      with the last datum before them. For sorted data, the variance returned
      is the winsorized sample variance.

      \returns
      -# Neither, if no data remain beyond those replaced;
      -# Only the mean, if there is a single datum;
      -# Both, otherwise.
   */
  template<class T = void, statistics_expressible_in<T> Data>
  [[nodiscard]]
  variance_and_mean<statistic_value_type_t<T, Data>>
    winsorized_sample_variance(Data&& data, std::size_t numReplacedAtEachEnd)
  {
    using statistic_type = statistic_value_type_t<T, Data>;
    using working_type   = impl::statistic_working_type_t<T, Data>;

    const auto dist{std::ranges::distance(data)};
    if(std::cmp_greater_equal(numReplacedAtEachEnd, dist - dist / 2))
    {
      return {};
    }

    const auto firstKeptIndex{static_cast<std::ranges::range_difference_t<Data>>(numReplacedAtEachEnd)},
               finalKeptIndex{dist - firstKeptIndex - 1};

    const auto firstKept{std::ranges::next(std::ranges::begin(data), firstKeptIndex)},
               finalKept{std::ranges::next(firstKept, finalKeptIndex - firstKeptIndex)};

    const std::ranges::subrange kept{firstKept, std::ranges::next(finalKept)};

    const auto lowest {static_cast<working_type>(*firstKept)},
               highest{static_cast<working_type>(*finalKept)};

    const working_type winsorizedMean{
        (  std::ranges::fold_left(kept, working_type{}, std::plus<>{})
         + static_cast<working_type>(numReplacedAtEachEnd) * (lowest + highest))
      / static_cast<working_type>(dist)
    };

    if(dist == 1)
    {
      return {.mean{static_cast<statistic_type>(winsorizedMean)}};
    }

    auto squareDiff{
      [winsorizedMean](working_type datum) { return (datum - winsorizedMean)*(datum - winsorizedMean); }
    };

    auto addSquareDiff{
      [squareDiff](working_type sum, working_type datum) { return sum + squareDiff(datum); }
    };

    const working_type sumOfSquareDiffs{
        std::ranges::fold_left(kept, working_type{}, addSquareDiff)
      + static_cast<working_type>(numReplacedAtEachEnd) * (squareDiff(lowest) + squareDiff(highest))
    };

    return {
      .variance{static_cast<statistic_type>(sumOfSquareDiffs / static_cast<working_type>(dist - 1))},
      .mean{static_cast<statistic_type>(winsorizedMean)}
    };
  }

  /** \brief Returns the population standard deviation and the mean of
             `data`.

      \returns
      -# Neither, if `data` is empty;
      -# Both, otherwise.
   */
  template<class T = void, statistics_expressible_in<T> Data>
  [[nodiscard]]
  standard_deviation_and_mean<statistic_value_type_t<T, Data>> standard_deviation(Data&& data)
  {
    using statistic_type = statistic_value_type_t<T, Data>;
    using working_type   = impl::statistic_working_type_t<T, Data>;

    if(std::ranges::distance(data))
    {
      const auto [var, mean]{maths::variance<working_type>(data)};

      return {
        .standard_deviation{static_cast<statistic_type>(std::sqrt(var.value()))},
        .mean{static_cast<statistic_type>(mean.value())}
      };
    }

    return {};
  }

  namespace bias
  {
    /** \brief An estimator of the standard deviation of a normally
               distributed population from a sample.

        For a sample \f$x_1, \ldots, x_n\f$ with mean \f$\bar{x}\f$, the
        estimate is \f$\sqrt{\sum_i (x_i - \bar{x})^2 / (n - 1.5)}\f$. For a
        normal population, dividing by \f$n - 1.5\f$ approximately removes
        the estimate's bias.
     */
    struct gaussian_approx_estimator
    {
      /** \brief Returns the estimate and the mean of `data`.

          \returns
          -# Neither, if `data` is empty;
          -# Only the mean, if there is a single datum;
          -# Both, otherwise.
       */
      template<class T = void, statistics_expressible_in<T> Data>
      [[nodiscard]]
      standard_deviation_and_mean<statistic_value_type_t<T, Data>> operator()(Data&& data) const
      {
        using statistic_type = statistic_value_type_t<T, Data>;
        using working_type   = impl::statistic_working_type_t<T, Data>;

        if(const auto dist{std::ranges::distance(data)}; !dist)
        {
          return {};
        }
        else if(dist == 1)
        {
          return {.mean{maths::mean<T>(data)}};
        }
        else
        {
          const auto [sq, mean]{maths::cumulative_square_diffs<working_type>(data)};
          const auto biasCorrection{static_cast<working_type>(1.5)};

          return {
            .standard_deviation{
              static_cast<statistic_type>(std::sqrt(sq.value() / (static_cast<working_type>(dist) - biasCorrection)))
            },
            .mean{static_cast<statistic_type>(mean.value())}
          };
        }
      }
    };
  }

  /** \brief Returns the result of invoking `estimator` on `data`.

      The call operator of `estimator` is invoked on `data`, with `T` as its
      first template argument, and must return a `standard_deviation_and_mean`
      of `statistic_value_type_t<T, Data>`. The default estimator returns its
      estimate of the population standard deviation, and the mean of `data`.
   */
  template<
    class T         = void,
    statistics_expressible_in<T> Data,
    class Estimator = bias::gaussian_approx_estimator
  >
    requires requires(Estimator& estimator, Data&& data) {
      { estimator.template operator()<T>(std::forward<Data>(data)) }
        -> std::same_as<standard_deviation_and_mean<statistic_value_type_t<T, Data>>>;
    }
  [[nodiscard]]
  standard_deviation_and_mean<statistic_value_type_t<T, Data>>
    sample_standard_deviation(Data&& data, Estimator estimator = Estimator{})
  {
    return estimator.template operator()<T>(std::forward<Data>(data));
  }
}
