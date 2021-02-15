////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file StatisticalAlgorithms.hpp
    \brief Tools for statistical analysis.
*/

#include <cmath>
#include <numeric>
#include <optional>

namespace sequoia::maths
{
  template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
  [[nodiscard]]
  std::optional<T> mean(InputIt first, InputIt last)
  {
    using std::distance;

    std::optional<T> m{};

    if(const auto dist{distance(first, last)})
    {
      m = std::accumulate(first, last, T{}) / dist;
    }

    return m;
  }

  template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    cummulative_square_diffs(InputIt first, InputIt last)
  {
    using std::distance;

    if(distance(first, last))
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

  template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    variance(InputIt first, InputIt last)
  {
    using std::distance;

    if(const auto dist{distance(first, last)})
    {
      auto [sq, mean]{cummulative_square_diffs(first, last)};

      return {sq.value()/dist, mean.value()};
    }

    return {{}, {}};
  }

  template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    sample_variance(InputIt first, InputIt last)
  {
    using std::distance;

    if(const auto dist{distance(first, last)}; !dist)
    {
      return {{}, {}};
    }
    else if(dist == 1)
    {
      return {{}, mean(first, last)};
    }
    else
    {
      auto [sq, mean]{cummulative_square_diffs(first, last)};

      return {sq.value()/(dist - 1), mean.value()};
    }
  }
  
  template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
    standard_deviation(InputIt first, InputIt last)
  {
    using std::distance;

    if(const auto dist{distance(first, last)})
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
      template<class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
      [[nodiscard]]
      std::pair<std::optional<T>, std::optional<T>>
        operator()(InputIt first, InputIt last) const
      {
        using std::distance;

        if(const auto dist{distance(first, last)}; !dist)
        {
          return {{}, {}};
        }
        else if(dist == 1)
        {
          return {{}, mean(first, last)};
        }
        else
        {
          auto [sq, mean]{cummulative_square_diffs(first, last)};

          return {std::sqrt(sq.value()/(dist - 1.5)), mean.value()};
        }
      }
    };
  }
    
  template<class InputIt, class Estimator = bias::gaussian_approx_estimator, class T = typename std::iterator_traits<InputIt>::value_type>
  [[nodiscard]]
  std::pair<std::optional<T>, std::optional<T>>
  sample_standard_deviation(InputIt first, InputIt last, Estimator estimator = Estimator{})
  {
    return estimator(first, last);
  }
}
