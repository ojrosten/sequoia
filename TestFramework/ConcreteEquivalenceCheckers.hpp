////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Useful specializations for the class template equivalence_checker.
*/

#include "FreeCheckers.hpp"
#include "Concepts.hpp"

#include <tuple>

namespace sequoia::testing
{
  /*! \brief Checks equivalence of std::basic_string to char[] and string_view */
  template<class Char, class Traits, alloc Allocator>
  struct equivalence_checker<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;
    
    template<test_mode Mode, std::size_t N, class Advisor>
    static void check(test_logger<Mode>& logger, const string_type& s, char const (&prediction)[N], Advisor advisor)
    {
      check_equality("", logger, std::string_view{s}, std::string_view{prediction}, std::move(advisor));
    }

    template<test_mode Mode, class Advisor>
    static void check(test_logger<Mode>& logger, const string_type& s, std::basic_string_view<Char, Traits> prediction, Advisor advisor)
    {
      check_equality("", logger, std::string_view{s}, prediction, std::move(advisor));
    }
  };

  /*! \brief Checks equivalence of std::pair<S,T> and std::pair<U,V> where,
      after removing references and cv-qualifiers, S,U and T,V are each the same
   */
  template<class S, class T>
  struct equivalence_checker<std::pair<S, T>>
  {
    template<test_mode Mode, class U, class V, class Advisor>
    static void check(test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction, const Advisor& advisor)
    {        
      static_assert(   std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<U>>
                    && std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<V>>);

      check_equality("First element of pair is incorrect", logger, value.first, prediction.first, advisor);
      check_equality("Second element of pair is incorrect", logger, value.second, prediction.second, advisor);
    }
  };

  /*! \brief Checks equivalence of std::tuple<T...> and std::tuple<U...> where T... and U...
      are the same size and, after removing references and cv-qualifiers, the respective elements
      are of the same type
   */
  template<class... T>
  struct equivalence_checker<std::tuple<T...>>
  {
  private:
    template<test_mode Mode, std::size_t I = 0, class... U, class Advisor>
    static void check_tuple_elements(test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const Advisor& advisor)
    {
      if constexpr(I < sizeof...(T))
      {
        const std::string message{"Element " + std::to_string(I) + " of tuple incorrect"};
        check_equality(message, logger, std::get<I>(value), std::get<I>(prediction), advisor);
        check_tuple_elements<Mode, I+1>(logger, value, prediction, advisor);
      }
    }
      
  public:
    template<test_mode Mode, class... U, class Advisor>
    static void check(test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, Advisor advisor)
    {
      static_assert(sizeof...(T) == sizeof...(U));
      static_assert((std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>> && ...));      

      check_tuple_elements(logger, value, prediction, advisor);
    }
  };
}
