////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Contains utilities for the advice framework.
 */

#include "Concepts.hpp"
#include "TypeTraits.hpp"
#include "Format.hpp"

#include <string>

namespace sequoia::testing
{
  /// \brief Represents the absence of advice
  struct null_advisor
  {};

  /*! \brief class template used to wrap function objects which proffer advice.

      An appropriate instantiation of this class template may be supplied as the
      final argument of many of the check methods. For example, consider an int, x:

      check_equality(LINE(""), x, 41, tutor{[](double value, double prediction) {\n
        return value == 42 ? "Are you sure the universe isn't trying to tell you something?" : "";\n
      }});\n

      In the case the x != 41, not only will failure be reported in the usual manner
      but, if x == 42, some spectacularly useful advice will be proffered.

      \anchor tutor_primary
   */
  template<class Advisor>
  class tutor
  {
  public:
    using sequoia_advisor_type = Advisor;
    
    tutor(Advisor a)
      : m_Advisor{std::move(a)}
    {}

    template<class T>
      requires invocable<Advisor, T, T>
    [[nodiscard]]
    std::string operator()(const T& value, const T& prediction) const
    {
      return m_Advisor(value, prediction);
    }
  private:
    Advisor m_Advisor;
  };

  template<>
  class tutor<null_advisor>
  {
  public:
    using sequoia_advisor_type = null_advisor;
  };

  template<class A>
  concept teacher = requires() {
    typename std::remove_cvref_t<A>::sequoia_advisor_type;
  };

  template<class... U>
  struct ends_with_tutor : std::false_type
  {};

  template<class... U>
  requires    (sizeof...(U) > 0u)
           && teacher<decltype(std::get<sizeof...(U) - 1>(std::declval<std::tuple<U...>>()))>
  struct ends_with_tutor<U...> : std::true_type
  {};


  template<class... U>
  constexpr bool ends_with_tutor_v{ends_with_tutor<U...>::value};

  class advice_data
  {
  public:
    template<class Advisor, class T>
    advice_data(const tutor<Advisor>& advisor, const T& value, const T& prediction)
    {
      if constexpr(std::is_invocable_r_v<std::string, tutor<Advisor>, T, T>)
      {
        m_Advice = advisor(value, prediction);
      }

      
      m_AdviceTypeName = type_demangler<Advisor>::make();
    }

    void append_and_tidy(std::string& message) const;    
  private:
    std::string m_Advice{}, m_AdviceTypeName{};
    
    void tidy(std::string& message) const;
  };

  void append_advice(std::string& message, const advice_data& adviceData);
}
