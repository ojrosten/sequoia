////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \brief Extension of unit testing framework for inexact comparisons.
 
    This header provides utilities for performing a comparison between two instances of
    a type utilising a generic function object. A particular use-case is comparison
    within a tolerance, for which a concrete function object is supplied.
    
 */

#include "RegularTestCore.hpp"

#include <cmath>

namespace sequoia::unit_testing
{
  namespace impl
  {
    template<class Compare>
    struct fuzzy_compare
    {
      explicit fuzzy_compare(Compare c) : compare{std::move(c)} {}
      
      Compare compare;
    };

    template<class Compare, class T, class=std::void_t<>>
    struct compares_type : std::false_type
    {};

    template<class Compare, class T>
    struct compares_type<Compare, T, std::void_t<std::invoke_result_t<Compare, T, T>>>
      : std::true_type
    {};

    template<class Compare, class T>
    constexpr bool compares_type_v{compares_type<Compare, T>::value};

    template<class Compare, class T, class=std::void_t<>>
    struct reports_for_type : std::false_type
    {};

    template<class Compare, class T>
    struct reports_for_type<Compare, T, std::void_t<decltype(std::declval<Compare>().report(std::declval<T>(), std::declval<T>()))>>
      : std::true_type
    {};

    template<class Compare, class T>
    constexpr bool reports_for_type_v{reports_for_type<Compare, T>::value};    
  }
  
  template<test_mode Mode, class Compare, class T>
  bool dispatch_check(std::string_view description, unit_test_logger<Mode>& logger, impl::fuzzy_compare<Compare> c, const T& value, const T& prediction)
  {
    using sentinel = typename unit_test_logger<Mode>::sentinel;      
    sentinel s{logger, add_type_info<T>(description)};

    if constexpr(impl::compares_type_v<Compare, T>)
    {
      const auto priorFailures{logger.failures()};

      s.log_check();
      if(!c.compare(prediction, value))
      {
        std::string message{};
        if(!description.empty())
          message.append("\t").append(description).append("\n");
        
        message.append(add_type_info<T>(""));
        if constexpr(impl::reports_for_type_v<Compare, T>)
        {
          message.append(c.compare.report(value, prediction));
        }
        else
        {
          message.append("\tFuzzy comparison failed\n\tObtained : ")
            .append(to_string(value))
            .append("\n\tPredicted: ")
            .append(to_string(prediction))
            .append("\n\n");         
        }

        logger.log_failure(message);
      }

      return logger.failures() == priorFailures;
    }
    else if constexpr(is_container_v<T>)
    {
      return check_range(description, logger, std::move(c), value.begin(), value.end(), prediction.begin(), prediction.end());
    }
    else
    {
      static_assert(dependent_false<T>::value, "Compare cannot consume T directly nor interpret as a range");
    }
  }

  //================= namespace-level convenience functions =================//

  template<test_mode Mode, class Compare, class T>
  bool check_approx_equality(std::string_view description, unit_test_logger<Mode>& logger, Compare&& compare, const T& value, const T& prediction)
  {
    return dispatch_check(description, logger, impl::fuzzy_compare<Compare>{compare}, value, prediction);
  }

  template<test_mode Mode, class Iter, class PredictionIter, class Compare>
  bool check_range_approx(std::string_view description, unit_test_logger<Mode>& logger, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return check_range(description, logger, impl::fuzzy_compare{std::move(compare)}, first, last, predictionFirst, predictionLast);      
  }

  template<test_mode Mode>
  class fuzzy_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    explicit fuzzy_extender(unit_test_logger<Mode>& logger) : m_Logger{logger} {}

    fuzzy_extender(const fuzzy_extender&) = delete;

    fuzzy_extender& operator=(const fuzzy_extender&) = delete;
    fuzzy_extender& operator=(fuzzy_extender&&)      = delete;

    template<class T, class Compare>
    bool check_approx_equality(std::string_view description, Compare compare, const T& value, const T& prediction)
    {
      return unit_testing::check_approx_equality(description, m_Logger, std::move(compare), value, prediction);      
    }

    template<class Iter, class PredictionIter, class Compare>
    bool check_range_approx(std::string_view description, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return unit_testing::check_range_approx(description, m_Logger, std::move(compare), first, last, predictionFirst, predictionLast);      
    }

  protected:    
    fuzzy_extender(fuzzy_extender&&) noexcept = default;
    ~fuzzy_extender() = default;

  private:
    unit_test_logger<Mode>& m_Logger;
  };

  template<test_mode mode>
  using fuzzy_checker = checker<mode, fuzzy_extender<mode>>;
  
  template<test_mode mode>
  using basic_fuzzy_test = basic_test<fuzzy_checker<mode>>;

  using fuzzy_test                = basic_fuzzy_test<test_mode::standard>;
  using fuzzy_false_negative_test = basic_fuzzy_test<test_mode::false_negative>;
  using fuzzy_false_positive_test = basic_fuzzy_test<test_mode::false_positive>;

  template<class T>
  class within_tolerance
  {
    T m_Tol{};
  public:
    using value_type = T;
    
    constexpr explicit within_tolerance(T tol) : m_Tol{std::move(tol)} {};

    [[nodiscard]]
    constexpr bool operator()(const T& value, const T& prediction) const noexcept
    {
      using std::abs;
      return abs(value - prediction) <= m_Tol;
    }

    [[nodiscard]]
    std::string report(const T& value, const T& prediction) const
    {
      std::string mess{"\tObtained : " };
      mess.append(to_string(value))
        .append("\n\tPredicted: ")
        .append(to_string(prediction))              
        .append(" +/- ")
        .append(to_string(m_Tol))
        .append("\n");

      return mess;
    }
  };
}
