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
    a range. Thus if, for example, a fuzzy comparison is defined for a type, T, then no
    additional work is required to do a fuzzy comparison of the elements of a container of T.
    All that is required to do this is to feed the fuzzy_compare object to the existing
    check_range overload set and everything works smoothly.
 */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include <cmath>

namespace sequoia::testing
{
  /*! Thin wrapper for the comparison object, for the purposes of cleanly adding an overload
      to \ref dispatch_check_free_overloads "dispatch_check".
   */
  template<class Compare>
  struct fuzzy_compare
  {
    explicit fuzzy_compare(Compare c) : compare{std::move(c)} {}
      
    Compare compare;
  };

  

  /*! \brief Concept for wether an object of type Compare defines a function, report, which accepts
      two instance of a type, T.

      Generally, when writing a new comparison class, it is desirable to provide detailed information
      in the case that the comparison fails. If this is done via a function, report, it will be
      automatically used by
      \ref dispatch_check_fuzzy "dispatch_check"
   */

  template<class Compare, class T>
  concept fuzzy_reporter = requires(Compare& c, const std::remove_reference_t<T>& t) {
   c.report(t, t);
  };

  /*! \brief Adds to the overload set dispatch_check_free_overloads */
  template<test_mode Mode, class Compare, class T, class Advisor>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, fuzzy_compare<Compare> c, const T& obtained, const T& prediction, tutor<Advisor> advisor)
  {  
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(invocable<Compare, T, T>)
    {
      sentry.log_check();
      if(!c.compare(prediction, obtained))
      {
        std::string message{};
        if constexpr(fuzzy_reporter<Compare, T>)
        {
          message = c.compare.report(obtained, prediction);
        }
        else
        {
          message = append_lines("Fuzzy comparison failed", prediction_message(to_string(obtained), to_string(prediction)));
        }
       
        append_advice(message, {advisor, obtained, prediction});
          
        sentry.log_failure(message);
      }

      return !sentry.failure_detected();
    }
    else if constexpr(range<T>)
    {
      return check_range("", logger, std::move(c), obtained.begin(), obtained.end(), prediction.begin(), prediction.end(), advisor);
    }
    else
    {
      static_assert(dependent_false<T>::value, "Compare cannot consume T directly nor interpret as a range");
    }
  }

  //================= namespace-level convenience functions =================//

  template<test_mode Mode, class Compare, class T, class Advisor>
  bool check_approx_equality(std::string_view description, test_logger<Mode>& logger, Compare&& compare, const T& obtained, const T& prediction, tutor<Advisor> advisor)
  {
    return dispatch_check(description, logger, fuzzy_compare<Compare>{compare}, obtained, prediction, std::move(advisor));
  }

  template<test_mode Mode, class Iter, class PredictionIter, class Compare, class Advisor>
  bool check_range_approx(std::string_view description, test_logger<Mode>& logger, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor)
  {
    return check_range(description, logger, fuzzy_compare{std::move(compare)}, first, last, predictionFirst, predictionLast, std::move(advisor));
  }

  /*! \brief class template for plugging into the \ref checker_primary "checker"
      class template to provide fuzzy checks.

      \anchor fuzzy_extender_primary
   */
  template<test_mode Mode>
  class fuzzy_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    explicit fuzzy_extender(test_logger<Mode>& logger) : m_Logger{logger} {}

    fuzzy_extender(const fuzzy_extender&)            = delete;
    fuzzy_extender& operator=(const fuzzy_extender&) = delete;

    template<class T, class Compare, class Advisor=null_advisor>
    bool check_approx_equality(std::string_view description, Compare compare, const T& obtained, const T& prediction, tutor<Advisor> advisor=tutor<Advisor>{})
    {
      return testing::check_approx_equality(description, m_Logger, std::move(compare), obtained, prediction, std::move(advisor));      
    }

    template<class Iter, class PredictionIter, class Compare, class Advisor=null_advisor>
    bool check_range_approx(std::string_view description, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor=tutor<Advisor>{})
    {
      return testing::check_range_approx(description, m_Logger, std::move(compare), first, last, predictionFirst, predictionLast, std::move(advisor));      
    }

  protected:
    fuzzy_extender(fuzzy_extender&&)             noexcept = default;
    fuzzy_extender& operator=(fuzzy_extender&&)  noexcept = default;

    ~fuzzy_extender() = default;

  private:
    test_logger<Mode>& m_Logger;
  };

  template<test_mode mode>
  using fuzzy_checker = checker<mode, fuzzy_extender<mode>>;
  
  template<test_mode mode>
  using basic_fuzzy_test = basic_test<fuzzy_checker<mode>>;

  /*! \anchor fuzzy_test_alias */
  using fuzzy_test                = basic_fuzzy_test<test_mode::standard>;
  using fuzzy_false_negative_test = basic_fuzzy_test<test_mode::false_negative>;
  using fuzzy_false_positive_test = basic_fuzzy_test<test_mode::false_positive>;

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
    constexpr bool operator()(const T& obtained, const T& prediction) const noexcept
    {
      using std::abs;
      return abs(obtained - prediction) <= m_Tol;
    }

    [[nodiscard]]
    std::string report(const T& obtained, const T& prediction) const
    {
      auto mess{prediction_message(to_string(obtained), to_string(prediction))
        .append(" +/- ")
        .append(to_string(m_Tol))};

      return mess;
    }
  };
}
