////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file StatisticalAlgorithms.hpp
    \brief Tools for statistical analysis.

    Every algorithm here traverses its range twice, once for the distance and once to accumulate,
    which is why they are constrained on `std::forward_iterator` rather than `std::input_iterator`:
    a single-pass iterator is consumed by the first traversal.
*/

#include <cmath>
#include <numeric>
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
      m = std::accumulate(first, last, T{}) / dist;
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
        std::accumulate(first, last, T{}, [m](const T& sum, const T& datum){
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
