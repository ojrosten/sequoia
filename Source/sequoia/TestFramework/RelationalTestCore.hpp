////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extension of testing framework for inexact comparisons.

    This header provides utilities for performing a comparison between two instances of
    a type utilising a generic function object. A particular use-case is comparison
    within a tolerance, for which a concrete function object is supplied.

    The pattern is to provide a new overload for
    \ref dispatch_check_free_overloads "dispatch_check". Internally, if no compare
    object is identified for the given type, an attempt it made to interpret the type as
    a range. Thus if, for example, a relational comparison is defined for a type, T, then no
    additional work is required to do a relational comparison of the elements of a container of T.
    All that is required to do this is to feed the relational_compare object to the existing
    check_range overload set and everything works smoothly.
 */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <cmath>
#include <functional>

namespace sequoia::testing
{
  //================= namespace-level convenience functions =================//

  

  /*template<test_mode Mode, class Iter, class PredictionIter, class Compare, class Advisor>
  bool check_range_relation(std::string_view description, test_logger<Mode>& logger, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor)
  {
    return check_range(description, logger, std::move(compare), first, last, predictionFirst, predictionLast, std::move(advisor));
    }*/

  /*! \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide relational checks.

      \anchor relational_extender_primary
   */
  template<test_mode Mode>
  class relational_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    explicit relational_extender(test_logger<Mode>& logger) : m_Logger{logger} {}

    relational_extender(const relational_extender&)            = delete;
    relational_extender& operator=(const relational_extender&) = delete;

  protected:
    relational_extender(relational_extender&&)             noexcept = default;
    relational_extender& operator=(relational_extender&&)  noexcept = default;

    ~relational_extender() = default;

  private:
    test_logger<Mode>& m_Logger;
  };

  template<test_mode mode>
  using relational_checker = checker<mode, relational_extender<mode>>;

  template<test_mode mode>
  using basic_relational_test = basic_test<relational_checker<mode>>;

  /*! \anchor relational_test_alias */
  using relational_test                = basic_relational_test<test_mode::standard>;
  using relational_false_negative_test = basic_relational_test<test_mode::false_negative>;
  using relational_false_positive_test = basic_relational_test<test_mode::false_positive>;

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
