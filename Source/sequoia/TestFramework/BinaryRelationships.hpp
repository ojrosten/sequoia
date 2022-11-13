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
    template<bool IsFinalMessage, class T>
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const Compare&, const T&, const T&) = delete;
  };

  /*! \brief Function object for performing comparisons within an absolute tolerance

      \anchor within_tolerance_primary
   */
  template<std::totally_ordered ToleranceType>
  class within_tolerance
  {
    ToleranceType m_Tol{};
  public:
    using tolerance_type = ToleranceType;

    constexpr explicit within_tolerance(ToleranceType tol) : m_Tol{std::move(tol)} {};

    template<class T>
    constexpr static bool has_std_abs{requires(const T& x) { { std::abs(x) } -> std::same_as<ToleranceType>; }};

    template<class T>
    constexpr static bool has_adl_abs{requires(const T& x) { { abs(x) } -> std::same_as<ToleranceType>; }};

    [[nodiscard]]
    constexpr const tolerance_type& tol() const noexcept
    {
      return m_Tol;
    }

    template<class T>
      requires (has_std_abs<T> || has_adl_abs<T>)
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
    template<bool IsFinalMessage, class ComparedType>
      requires reportable<ComparedType>
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const within_tolerance<T>& c, const ComparedType& obtained, const ComparedType& prediction)
    {
      return prediction_message(obtained, prediction).append(" +/- ").append(to_string(c.tol()));
    }
  };

  template<class T>
  struct failure_reporter<std::equal_to<T>>
  {
    template<bool IsFinalMessage>
      requires (reportable<T> || !IsFinalMessage)
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const std::equal_to<T>&, const T& obtained, const T& prediction)
    {
      return failure_message(final_message_constant<IsFinalMessage>{}, obtained, prediction);
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
    template<bool IsFinalMessage>
      requires reportable<T>
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const std::less<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message("<", obtained, prediction);
    }
  };

  template<class T>
  struct failure_reporter<std::less_equal<T>>
  {
    template<bool IsFinalMessage>
      requires reportable<T>
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const std::less_equal<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message("<=", obtained, prediction);
    }
  };

  template<class T>
  struct failure_reporter<std::greater<T>>
  {
    template<bool IsFinalMessage>
      requires reportable<T>
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const std::greater<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message(">", obtained, prediction);
    }
  };

  template<class T>
  struct failure_reporter<std::greater_equal<T>>
  {
    template<bool IsFinalMessage>
      requires reportable<T>
    [[nodiscard]]
    static std::string report(final_message_constant<IsFinalMessage>, const std::greater_equal<T>&, const T& obtained, const T& prediction)
    {
      return relational_failure_message(">=", obtained, prediction);
    }
  };
}
