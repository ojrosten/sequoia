////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for the advice framework.
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/TestFramework/Output.hpp"

#include <string>

namespace sequoia::testing
{
  template<class Advisor>
  struct advisor_invoke_type;

  template<class R, class Advisor, class T>
  struct advisor_invoke_type<R (Advisor::*)(T, T)>
  {
    using return_type = R;
    using type = T;
  };

  template<class R, class Advisor, class T>
  struct advisor_invoke_type<R (Advisor::*)(T, T) const>
  {
    using return_type = R;
    using type = T;
  };

  template<class R, class Advisor, class T>
  struct advisor_invoke_type<R (Advisor::*)(T, T) const noexcept>
  {
    using return_type = R;
    using type = T;
  };

  template<class R, class Advisor, class T>
  struct advisor_invoke_type<R (Advisor::*)(T, T) noexcept>
  {
    using return_type = R;
    using type = T;
  };

  template<class Advisor, class T>
  struct advisor_analyser
  {
    constexpr static bool utilize{false};
  };

  template<class Advisor, class T>
    requires std::invocable<Advisor, T, T>
  struct advisor_analyser<Advisor, T>
  {
    constexpr static bool utilize{true};
  };

  // For a Advisor with a single operator(), attempt to disallow bindings which involve
  // a narrowing conversion
  template<class Advisor, class T>
    requires std::invocable<Advisor, T, T> && requires {
      std::declval<decltype(&Advisor::operator())>();
    }
  struct advisor_analyser<Advisor, T>
  {
  private:
    using type = std::remove_cvref_t<typename advisor_invoke_type<decltype(&Advisor::operator())>::type>;
  public:
    constexpr static bool utilize{std::is_same_v<std::common_type_t<type, std::remove_cvref_t<T>>, type>};
  };

  template<class Advisor, class T>
  constexpr bool use_advisor_v{advisor_analyser<Advisor, T>::utilize};

  /// \brief Represents the absence of advice
  struct null_advisor
  {};

  /*! \brief class template used to wrap function objects which proffer advice.

      An appropriate instantiation of this class template may be supplied as the
      final argument of many of the check methods. For example, consider an int, x:

      <pre>
      check(equality, LINE(""), x, 41, tutor{[](double value, double prediction) {
          return value == 42 ? "Are you sure the universe isn't trying to tell you something?" : "";
      }});
      </pre>

      In the case the x != 41, not only will failure be reported in the usual manner
      but, if x == 42, some spectacularly useful advice will be proffered.

      \anchor tutor_primary
   */
  template<class Advisor>
  class tutor
  {
  public:
    using sequoia_advisor_type = Advisor;

    tutor(Advisor a, std::string prefix="Advice: ")
      : m_Advisor{std::move(a)}
      , m_Prefix{std::move(prefix)}
    {}

    template<class T>
      requires use_advisor_v<Advisor, T>
    [[nodiscard]]
    std::string operator()(const T& value, const T& prediction) const
    {
      return m_Advisor(value, prediction);
    }

    [[nodiscard]]
    const std::string& prefix() const noexcept
    {
      return m_Prefix;
    }
  private:
    Advisor m_Advisor;
    std::string m_Prefix;
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
             && teacher<decltype(std::get<sizeof...(U) - 1>(std::declval<std::tuple<std::remove_cvref_t<U>&...>>()))>
  struct ends_with_tutor<U...> : std::true_type
  {};


  template<class... U>
  inline constexpr bool ends_with_tutor_v{ends_with_tutor<U...>::value};

  class advice_data
  {
  public:
    template<class Advisor, class T>
    advice_data(const tutor<Advisor>& advisor, const T& value, const T& prediction)
    {
      if constexpr(std::is_invocable_r_v<std::string, tutor<Advisor>, T, T>)
      {
        m_Advice = advisor(value, prediction);
        m_Prefix = advisor.prefix();
      }
    }

    std::string& append_to(std::string& message) const;
  private:
    std::string m_Advice{}, m_Prefix{};
  };

  std::string& append_advice(std::string& message, const advice_data& adviceData);
}
