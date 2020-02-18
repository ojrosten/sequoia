////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file FuzzyTestCore.hpp
    \brief Extension of unit testing framework for fuzzy testing.
*/

#include "UnitTestCore.hpp"

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
  }
  
  template<class Logger, class T, class Compare>
  bool check_approx_equality(std::string_view description, Logger& logger, Compare compare, const T& value, const T& prediction)
  {
    return check(description, logger, compare(value, prediction));
  }

  template<class Logger, class Iter, class PredictionIter, class Compare>
  bool check_range_approx(std::string_view description, Logger& logger, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return impl::check_range(description, logger, impl::fuzzy_compare{compare}, first, last, predictionFirst, predictionLast);      
  }


  // TO DO: put in impl namespace
  template<class Logger, class Compare, class T>
  bool check(std::string_view description, Logger& logger, impl::fuzzy_compare<Compare> c, const T& value, const T& prediction)
  {
    return check_approx_equality(description, logger, std::move(c.compare), value, prediction);
  }

  template<class Logger>
  class fuzzy_extender
  {
  public:
    explicit fuzzy_extender(Logger& logger) : m_Logger{logger} {}

    fuzzy_extender(const fuzzy_extender&) = delete;
    fuzzy_extender(fuzzy_extender&&)      = delete;

    fuzzy_extender& operator=(const fuzzy_extender&) = delete;
    fuzzy_extender& operator=(fuzzy_extender&&)      = delete;

    template<class T, class Compare>
    bool check_approx_equality(std::string_view description, Compare compare, const T& value, const T& prediction)
    {
      if constexpr(impl::compares_type_v<Compare, T>)
      {
        return unit_testing::check_approx_equality(description, m_Logger, std::move(compare), value, prediction);
      }
      else if constexpr(is_container_v<T> && impl::compares_type_v<Compare, decltype(*prediction.begin())>)
      {
        return check_range_approx(description, std::move(compare), value.begin(), value.end(), prediction.begin(), prediction.end());
      }
      else
      {
        static_assert(dependent_false<T>::value, "Compare cannot consume T");
      }
    }

    template<class Iter, class PredictionIter, class Compare>
    bool check_range_approx(std::string_view description, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return unit_testing::check_range_approx(description, m_Logger, std::move(compare), first, last, predictionFirst, predictionLast);      
    }

  protected:
    ~fuzzy_extender() = default;

  private:
    Logger& m_Logger;
  };

  template<test_mode mode>
  using fuzzy_checker = checker<unit_test_logger<mode>, fuzzy_extender<unit_test_logger<mode>>>;
  
  template<test_mode mode>
  using basic_fuzzy_test = basic_test<unit_test_logger<mode>, fuzzy_checker<mode>>;

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
      return  (value >= prediction - m_Tol) && (value <= prediction + m_Tol);
    }        
  };
}
