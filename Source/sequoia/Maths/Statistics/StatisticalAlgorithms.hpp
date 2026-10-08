////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file StatisticalAlgorithms.hpp
    \brief Tools for statistical analysis.
*/

#include <algorithm>
#include <cmath>
#include <functional>
#include <optional>
#include <ranges>
#include <utility>

namespace sequoia::maths
{
  /** \brief Returns the mean of `data`.

      There is no mean if `data` is empty.
   */
  template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
  [[nodiscard]]
  std::optional<T> mean(Data&& data)
  {
    std::optional<T> m{};

    if(const auto dist{std::ranges::distance(data)})
    {
      m = std::ranges::fold_left(data, T{}, std::plus<>{}) / dist;
    }

    return m;
  }

  /** \brief Returns the sum of the squared deviations of `data` from its mean,
             and the mean.

      \returns
      -# Neither, if `data` is empty;
      -# Both, otherwise.
   */
  template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    cumulative_square_diffs(Data&& data)
  {
    if(std::ranges::distance(data))
    {
      const auto m{maths::mean(data)};
      const auto var{
        std::ranges::fold_left(data, T{}, [m](const T& sum, const T& datum){
            return sum + (datum - m.value())*(datum - m.value());
          }
        )
      };

      return {var, m};
    }

    return {{}, {}};
  }

  /** \brief Returns the population variance and the mean of `data`.

      \returns
      -# Neither, if `data` is empty;
      -# Both, otherwise.
   */
  template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    variance(Data&& data)
  {
    if(const auto dist{std::ranges::distance(data)})
    {
      auto [sq, mean]{maths::cumulative_square_diffs(data)};

      return {sq.value()/dist, mean.value()};
    }

    return {{}, {}};
  }

  /** \brief Returns the sample variance and the mean of `data`.

      \returns
      -# Neither, if `data` is empty;
      -# Only the mean, if there is a single datum;
      -# Both, otherwise.
   */
  template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    sample_variance(Data&& data)
  {
    if(const auto dist{std::ranges::distance(data)}; !dist)
    {
      return {{}, {}};
    }
    else if(dist == 1)
    {
      return {{}, maths::mean(data)};
    }
    else
    {
      auto [sq, mean]{maths::cumulative_square_diffs(data)};

      return {sq.value()/(dist - 1), mean.value()};
    }
  }

  /** \brief Returns the sample variance and the mean of `data` once the
             first and last `numReplacedAtEachEnd` are winsorized.

      Winsorizing replaces each of the first `numReplacedAtEachEnd` data with
      the first datum after them, and each of the last `numReplacedAtEachEnd`
      with the last datum before them. For sorted data, the variance returned
      is the winsorized sample variance.

      \returns
      -# Neither, if `numReplacedAtEachEnd` is negative, or if no data remain
         beyond those replaced;
      -# Only the mean, if there is a single datum;
      -# Both, otherwise.
   */
  template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    winsorized_sample_variance(Data&& data, std::ranges::range_difference_t<Data> numReplacedAtEachEnd)
  {
    const auto dist{std::ranges::distance(data)};
    if((numReplacedAtEachEnd < 0) || (dist - numReplacedAtEachEnd <= numReplacedAtEachEnd))
    {
      return {{}, {}};
    }

    const auto numKept{dist - numReplacedAtEachEnd - numReplacedAtEachEnd};

    const auto firstKept{std::ranges::next(std::ranges::begin(data), numReplacedAtEachEnd)},
               finalKept{std::ranges::next(firstKept, numKept - 1)};

    const std::ranges::subrange kept{firstKept, std::ranges::next(finalKept)};

    const T lowest {*firstKept},
            highest{*finalKept};

    const T winsorizedMean{
        (std::ranges::fold_left(kept, T{}, std::plus<>{}) + numReplacedAtEachEnd * (lowest + highest))
      / dist
    };

    if(dist == 1)
    {
      return {{}, winsorizedMean};
    }

    auto squareDiff{[winsorizedMean](const T& datum) { return (datum - winsorizedMean)*(datum - winsorizedMean); }};
    auto addSquareDiff{[&squareDiff](const T& sum, const T& datum) { return sum + squareDiff(datum); }};

    const T cumulativeSquareDiffs{
        std::ranges::fold_left(kept, T{}, addSquareDiff)
      + numReplacedAtEachEnd * (squareDiff(lowest) + squareDiff(highest))
    };

    return {cumulativeSquareDiffs / (dist - 1), winsorizedMean};
  }

  /** \brief Returns the population standard deviation and the mean of
             `data`.

      \returns
      -# Neither, if `data` is empty;
      -# Both, otherwise.
   */
  template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    standard_deviation(Data&& data)
  {
    if(const auto dist{std::ranges::distance(data)})
    {
      auto [var, mean]{maths::variance(data)};

      return {std::sqrt(var.value()), mean.value()};
    }

    return {{}, {}};
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
      template<std::ranges::forward_range Data, class T = std::ranges::range_value_t<Data>>
      [[nodiscard]]
      std::pair<std::optional<T>, std::optional<T>>
        operator()(Data&& data) const
      {
        if(const auto dist{std::ranges::distance(data)}; !dist)
        {
          return {{}, {}};
        }
        else if(dist == 1)
        {
          return {{}, maths::mean(data)};
        }
        else
        {
          auto [sq, mean]{maths::cumulative_square_diffs(data)};

          return {std::sqrt(sq.value()/(dist - 1.5)), mean.value()};
        }
      }
    };
  }

  /** \brief Returns the result of invoking `estimator` on `data`.

      The default estimator returns its estimate of the population standard
      deviation, and the mean of `data`.
   */
  template<
    std::ranges::forward_range Data,
    class Estimator = bias::gaussian_approx_estimator,
    class T         = std::ranges::range_value_t<Data>
  >
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
  sample_standard_deviation(Data&& data, Estimator estimator = Estimator{})
  {
    return estimator(std::forward<Data>(data));
  }
}
