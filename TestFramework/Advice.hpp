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

  /// \brief class template used to wrap function objects which profer advice.
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
    typename A::sequoia_advisor_type;
  };

  template<class... U>
  constexpr bool has_tutor_v{
       ((sizeof...(U) == 1) && (teacher<head_of_t<U...>>))
    || ((sizeof...(U)  > 1) && (teacher<tail_of_t<U...>>))
  };

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
