////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Specializations of the class template value_tester for `std::variant`, `std::optional` and `std::any`.

    See ConcreteTypeCheckers.hpp for how nested checks dispatch, and for a single
    header pulling in every specialization at once.
 */

#include "sequoia/TestFramework/FreeCheckers.hpp"

#include <any>
#include <optional>
#include <variant>

namespace sequoia::testing
{
  /** \brief Compares instances of `std::variant` */

  template<class... Ts>
  struct value_tester<std::variant<Ts...>>
  {
    using type = std::variant<Ts...>;

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, tutor<Advisor> advisor)
    {
      if(check(equality, "Variant Index", logger, obtained.index(), prediction.index()))
      {
        check_value(flavour, logger, obtained, prediction, advisor, std::make_index_sequence<sizeof...(Ts)>());
      }
    }
  private:
    template<class CheckType, test_mode Mode, class Advisor, std::size_t... I>
    static void check_value(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor, std::index_sequence<I...>)
    {
      (check_value<I>(flavour, logger, obtained, prediction, advisor), ...);
    }

    template<std::size_t I, class CheckType, test_mode Mode, class Advisor>
    static void check_value(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      if(auto pObtained{std::get_if<I>(&obtained)})
      {
        if(auto pPrediction{std::get_if<I>(&prediction)})
        {
          check(flavour, "Variant Contents", logger, *pObtained, *pPrediction, advisor);
        }
        else
        {
          throw std::logic_error{"Inconsistant variant access"};
        }
      }
    }
  };

  /** \brief Compares instances of `std::optional` */

  template<class T>
  struct value_tester<std::optional<T>>
  {
    using type = std::optional<T>;

    template<class CheckType, test_mode Mode, class Advisor>
    static void test(CheckType flavour, test_logger<Mode>& logger, const type& obtained, const type& prediction, const tutor<Advisor>& advisor)
    {
      if(obtained && prediction)
      {
        check(flavour, "Contents of optional", logger, *obtained, *prediction, advisor);
      }
      else
      {
        const bool obtainedIsNull{obtained}, predictionIsNull{prediction};

        check(equality,
              nullable_type_message(obtainedIsNull, predictionIsNull),
              logger,
              static_cast<bool>(obtained),
              static_cast<bool>(prediction));
      }
    }
  };

  /** \brief Compares an instance of `std::any` to the value of the type it purportedly holds

      The semantics are such that, under the hood, `with_best_available` is utilized. Therefore,
      the equivalence of `std::any` to the value of a purported type may ultimately delegate
      to an `equality`/`equivalence`/`weak_equivalence` check.
   */

  template<>
  struct value_tester<std::any>
  {
    using type = std::any;

    template<test_mode Mode, class T, class Advisor>
    static void test(equivalence_check_t, test_logger<Mode>& logger, const type& obtained, const T& prediction, const tutor<Advisor>& advisor)
    {
      if(check("Has value", logger, obtained.has_value()))
      {
        try
        {
          const auto& val{std::any_cast<T>(obtained)};
          check(with_best_available, "Value held by std::any", logger, val, prediction, advisor);
        }
        catch(const std::bad_any_cast&)
        {
          check("std::any does not hold the expected type", logger, false);
        }
      }
    }
  };

}
