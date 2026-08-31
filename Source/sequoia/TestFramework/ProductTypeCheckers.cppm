////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

module;

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <concepts>
#include <execution>
#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <optional>
#include <scoped_allocator>
#include <source_location>
#include <span>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

export module sequoia.test_framework:ProductTypeCheckers;

import :Advice;
import :BinaryRelationships;
import :CoreInfrastructure;
import :FailureInfo;
import :FreeCheckers;
import :Output;
import :ProjectPaths;
import :TestLogger;
import :TestMode;
export import sequoia.core.meta;
export import sequoia.platform_specific;
export import sequoia.text_processing;

/** \file
    \brief Specializations of the class template value_tester for `std::pair` and `std::tuple`.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */



export namespace sequoia::testing
{
  /** \brief Compares instances of `std::pair` */

  template<class S, class T>
  struct value_tester<std::pair<S, T>>
  {
    template<class CheckType, test_mode Mode, class U, class V, class Advisor>
      requires (   std::is_same_v<std::remove_cvref_t<S>, std::remove_cvref_t<U>>
                && std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<V>>)
    static void test(CheckType flavour, test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction, const tutor<Advisor>& advisor)
    {
      check_elements(flavour, logger, value, prediction, std::move(advisor));
    }

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<S, T>& prediction, const tutor<Advisor>& advisor)
    {
      check_elements(equality, logger, value, prediction, std::move(advisor));
    }

  private:
    template<class CheckType, test_mode Mode, class U, class V, class Advisor>
    static void check_elements(CheckType, test_logger<Mode>& logger, const std::pair<S, T>& value, const std::pair<U, V>& prediction, const tutor<Advisor>& advisor)
    {
      check(CheckType{}, "First element of pair is incorrect", logger, value.first, prediction.first, advisor);
      check(CheckType{}, "Second element of pair is incorrect", logger, value.second, prediction.second, advisor);
    }
  };

  /** \brief Compares instances of `std::tuple` */

  template<class... T>
  struct value_tester<std::tuple<T...>>
  {
  private:
    template<std::size_t I = 0, class CheckType, test_mode Mode, class... U, class Advisor>
      requires (I < sizeof...(T))
    static void check_tuple_elements(CheckType flavour, test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const tutor<Advisor>& advisor)
    {
      check(flavour, std::format("Element {} of tuple incorrect", I), logger, std::get<I>(value), std::get<I>(prediction), advisor);
      check_tuple_elements<I+1>(flavour, logger, value, prediction, advisor);
    }

    template<std::size_t I = 0, class CheckType, test_mode Mode, class... U, class Advisor>
    static void check_tuple_elements(CheckType, test_logger<Mode>&, const std::tuple<T...>&, const std::tuple<U...>&, const tutor<Advisor>&)
    {}

  public:
    template<class CheckType, test_mode Mode, class... U, class Advisor>
      requires ((sizeof...(T) == sizeof...(U)) && (std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<U>> && ...))
    static void test(CheckType flavour, test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<U...>& prediction, const tutor<Advisor>& advisor)
    {
      check_tuple_elements(flavour, logger, value, prediction, advisor);
    }

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(equality_check_t, test_logger<Mode>& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, const tutor<Advisor>& advisor)
    {
      check_tuple_elements(equality, logger, value, prediction, advisor);
    }
  };

}
