////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for performing checks with respect to a binary operator

    This header provides utilities for performing a comparison between two instances of
    a type utilising a function object. Each such type must specialize
    \ref failure_reporter_primary "failure_reporter".
 */

#include "sequoia/TestFramework/Output.hpp"

#include <cmath>
#include <functional>

namespace sequoia::testing
{
   /*! \brief Specialize this struct template to provide custom reporting for comparisons
              performed with a binary operator.

      \anchor failure_reporter_primary
   */
  
  template<class Compare>
  struct failure_reporter
  {
    template<class T>
    [[nodiscard]]
    static std::string report(const Compare&, const T&, const T&) = delete;
  };

  
  /*! \brief Function object for performing comparisons within an absolute tolerance

      \anchor within_tolerance_primary
   */
  template<class T>
  class within_tolerance
  {
    T m_Tol{};
  public:
    using value_type = T;

    constexpr explicit within_tolerance(T tol) : m_Tol{std::move(tol)} {};

    [[nodiscard]]
    constexpr T tol() const noexcept
    {
      return m_Tol;
    }

    [[nodiscard]]
    constexpr bool operator()(const T& obtained, const T& prediction) const noexcept
    {
      using std::abs;
      return abs(obtained - prediction) <= m_Tol;
    }
  };

  template<class T>
  struct failure_reporter<within_tolerance<T>>
  {
    [[nodiscard]]
    static std::string report(const within_tolerance<T>& c, const T& obtained, const T& prediction)
    {
      return prediction_message(to_string(obtained), to_string(prediction))
        .append(" +/- ")
        .append(to_string(c.tol()));
    }
  };

  template<class T>
  [[nodiscard]]
  std::string relational_failure_message(std::string symbol, const T& obtained, const T& prediction)
  {
    return prediction_message(to_string(obtained), symbol.append(" ").append(to_string(prediction)));
  }

  template<class T>
  struct failure_reporter<std::less<T>>
  {
    [[nodiscard]]
    static std::string report(const std::less<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message("<", obtained, prediction);
    }
  };

  template<class T>
  struct failure_reporter<std::less_equal<T>>
  {
    [[nodiscard]]
    static std::string report(const std::less_equal<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message("<=", obtained, prediction);
    }
  };

  template<class T>
  struct failure_reporter<std::greater<T>>
  {
    [[nodiscard]]
    static std::string report(const std::greater<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message(">", obtained, prediction);
    }
  };

  template<class T>
  struct failure_reporter<std::greater_equal<T>>
  {
    [[nodiscard]]
    static std::string report(const std::greater_equal<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message(">=", obtained, prediction);
    }
  };
}
