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
#include <iterator>

namespace sequoia::maths
{
  template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::optional<T> mean(Iter first, Iter last)
  {
    std::optional<T> m{};

    if(const auto dist{std::ranges::distance(first, last)})
    {
      m = std::ranges::fold_left(first, last, T{}, std::plus<>{}) / dist;
    }

    return m;
  }

  template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    cumulative_square_diffs(Iter first, Iter last)
  {
    if(std::ranges::distance(first, last))
    {
      const auto m{mean(first, last)};
      const auto var{
        std::ranges::fold_left(first, last, T{}, [m](const T& sum, const T& datum){
            return sum + (datum - m.value())*(datum - m.value());
          }
        )
      };

      return {var, m};
    }

    return {{}, {}};
  }

  template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    variance(Iter first, Iter last)
  {
    if(const auto dist{std::ranges::distance(first, last)})
    {
      auto [sq, mean]{cumulative_square_diffs(first, last)};

      return {sq.value()/dist, mean.value()};
    }

    return {{}, {}};
  }

  template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    sample_variance(Iter first, Iter last)
  {
    if(const auto dist{std::ranges::distance(first, last)}; !dist)
    {
      return {{}, {}};
    }
    else if(dist == 1)
    {
      return {{}, mean(first, last)};
    }
    else
    {
      auto [sq, mean]{cumulative_square_diffs(first, last)};

      return {sq.value()/(dist - 1), mean.value()};
    }
  }

  /** \brief Returns the sample variance and the mean of the data once the
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
  template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    winsorized_sample_variance(Iter first, Iter last, std::iter_difference_t<Iter> numReplacedAtEachEnd)
  {
    const auto dist{std::ranges::distance(first, last)};
    if((numReplacedAtEachEnd < 0) || (dist - numReplacedAtEachEnd <= numReplacedAtEachEnd))
    {
      return {{}, {}};
    }

    const auto keptFirst{std::ranges::next(first, numReplacedAtEachEnd)},
               keptLast {std::ranges::next(first, dist - numReplacedAtEachEnd)};

    const T lowest{*keptFirst},
            highest{*std::ranges::prev(keptLast)};

    const T winsorizedMean{
        (std::ranges::fold_left(keptFirst, keptLast, T{}, std::plus<>{}) + numReplacedAtEachEnd * (lowest + highest))
      / dist
    };

    if(dist == 1)
    {
      return {{}, winsorizedMean};
    }

    auto squareDiff{[winsorizedMean](const T& datum) { return (datum - winsorizedMean)*(datum - winsorizedMean); }};
    auto addSquareDiff{[&squareDiff](const T& sum, const T& datum) { return sum + squareDiff(datum); }};

    const T cumulativeSquareDiffs{
        std::ranges::fold_left(keptFirst, keptLast, T{}, addSquareDiff)
      + numReplacedAtEachEnd * (squareDiff(lowest) + squareDiff(highest))
    };

    return {cumulativeSquareDiffs / (dist - 1), winsorizedMean};
  }

  template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    standard_deviation(Iter first, Iter last)
  {
    if(const auto dist{std::ranges::distance(first, last)})
    {
      auto [var, mean]{variance(first, last)};

      return {std::sqrt(var.value()), mean.value()};
    }

    return {{}, {}};
  }

  namespace bias
  {
    struct gaussian_approx_estimator
    {
      template<std::forward_iterator Iter, class T = typename std::iterator_traits<Iter>::value_type>
      [[nodiscard]]
      std::pair<std::optional<T>, std::optional<T>>
        operator()(Iter first, Iter last) const
      {
        if(const auto dist{std::ranges::distance(first, last)}; !dist)
        {
          return {{}, {}};
        }
        else if(dist == 1)
        {
          return {{}, mean(first, last)};
        }
        else
        {
          auto [sq, mean]{cumulative_square_diffs(first, last)};

          return {std::sqrt(sq.value()/(dist - 1.5)), mean.value()};
        }
      }
    };
  }

  template<std::forward_iterator Iter, class Estimator = bias::gaussian_approx_estimator, class T = typename std::iterator_traits<Iter>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
  sample_standard_deviation(Iter first, Iter last, Estimator estimator = Estimator{})
  {
    return estimator(first, last);
  }
}
