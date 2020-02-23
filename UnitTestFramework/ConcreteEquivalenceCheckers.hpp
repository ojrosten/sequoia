////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCheckers.hpp"

namespace sequoia::unit_testing
{
  template<class Char, class Traits, class Allocator>
  struct equivalence_checker<std::basic_string<Char, Traits, Allocator>>
  {
    using string_type = std::basic_string<Char, Traits, Allocator>;
    
    template<class Logger, std::size_t N>
    static void check(std::string_view description, Logger& logger, const string_type& s, char const (&prediction)[N])
    {
      check_equality(description, logger, std::string_view{s}, std::string_view{prediction});
    }

    template<class Logger>
    static void check(std::string_view description, Logger& logger, const string_type& s, std::basic_string_view<Char, Traits> prediction)
    {
      check_equality(description, logger, std::string_view{s}, prediction);
    }
  };

  template<class S, class T>
  struct equivalence_checker<std::pair<S, T>>
  {
    template<class Logger, class U, class V>
    static void check(std::string_view description, Logger& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction)
    {        
      static_assert(std::is_same_v<std::decay_t<S>, std::decay_t<U>> && std::is_same_v<std::decay_t<T>, std::decay_t<V>>);

      check_equality(combine_messages(description, "First element of pair is incorrect", "\n"), logger, value.first, prediction.first);
      check_equality(combine_messages(description, "Second element of pair is incorrect", "\n"), logger, value.second, prediction.second);
    }
  };

  template<class... T>
  struct equivalence_checker<std::tuple<T...>>
  {
  private:
    template<class Logger, std::size_t I = 0, class... U>
    static void check_tuple_elements(std::string_view description, Logger& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction)
    {
      if constexpr(I < sizeof...(T))
      {
        const std::string message{"Element " + std::to_string(I) + " of tuple incorrect"};
        check_equality(combine_messages(description, message, "\n"), logger, std::get<I>(value), std::get<I>(prediction));
        check_tuple_elements<Logger, I+1>(description, logger, value, prediction);
      }
    }
      
  public:
    template<class Logger, class... U>
    static void check(std::string_view description, Logger& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction)
    {        
      static_assert(sizeof...(T) == sizeof...(U));
      static_assert((std::is_same_v<std::decay_t<T>, std::decay_t<U>> && ...));      

      check_tuple_elements(description, logger, value, prediction);
    }
  };
}
