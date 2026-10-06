////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Utilities for the advice framework, which provides hints for certain failures.

    When a check fails, there may be some instances where the nature of the failure
    deserves comment. Indeed, such comments may provide a useful clue as to the origin
    of the failure, particularly when it is subtle. The advice framework facilitates this,
    allowing value-dependent comments to be generated in the instance of failure. This is
    achieved by passing an instance of \ref tutor_primary "tutor" to the appropriate
    `check` call.
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/TestFramework/Output.hpp"

namespace sequoia::testing
{
  /// \brief meta utility for garnering invocation and return types from an Advisor
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

  namespace impl
  {
    template<class Advisor>
    using advisor_argument_t = std::remove_cvref_t<typename advisor_invoke_type<decltype(&Advisor::operator())>::type>;
  }

  /// \brief meta utility for determining whether a particular Advisor should be used for a given type
  template<class Advisor, class T>
  struct use_advisor : std::false_type
  {};

  template<class Advisor, class T>
    requires std::invocable<Advisor, T, T>
  struct use_advisor<Advisor, T> : std::true_type
  {};

  // Attempt to disallow bindings which involve a narrowing conversion. I can only think
  // how to do this in the case of a single operator(), hence this specialization.
  // The logic to prohibit narrowing is in the value of this specialization.
  template<class Advisor, class T>
    requires std::invocable<Advisor, T, T> && requires {
      std::declval<decltype(&Advisor::operator())>();
    }
  struct use_advisor<Advisor, T>
    : std::bool_constant<
        std::is_same_v<
          std::common_type_t<impl::advisor_argument_t<Advisor>, std::remove_cvref_t<T>>,
          impl::advisor_argument_t<Advisor>
        >
      >
  {};

  template<class Advisor, class T>
  inline constexpr bool use_advisor_v{use_advisor<Advisor, T>::value};

  /// \brief Represents the absence of advice
  struct null_advisor
  {};

  /** \brief Class template used to wrap function objects which proffer advice.

      An appropriate instantiation of this class template may be supplied as
      the final argument of many of the check methods. For example, consider
      checking equality of an `int`:

      <pre>
      check(
        equality,
        "",
        x,
        41,
        tutor{
          [](int value, int prediction) {
            return value == 42 ? "Are you sure the universe isn't trying to tell you something?" : "";
          }
        }
      );
      </pre>

      In the case that `x != 41`, not only will failure be reported in the
      usual manner but, if `x == 42`, some spectacularly useful advice will be
      proffered.

      Matters are similar, though somewhat more subtle, for types which
      specialize \ref value_tester_primary "value_tester". Consider some type,
      `T`, for which this is done. Perhaps `T` wraps an `int`, in which case
      the specialization of `value_tester` will invoke a `check` for `int`s.
      In this case, there are several pieces of advice that could be produced,
      since the `tutor` is propagated down through the nested checks, and is
      invoked wherever the values are accepted by its wrapped advisor.

      -# If the `tutor` is of the same form as above, then the same advice
         will be proffered, at the point where the `value_tester` performs a
         `check` of the wrapped `int`s.

      -# If the `tutor` is constructed from an `Advisor` that supplies an
         overload of `operator()` that accepts two values of type `T`, then
         advice will be produced when the top-level comparison of the two
         values using `operator==` is performed.

      These two cases are not mutually exclusive. The philosophy is that as
      the check for a type is decomposed into its constituent parts, `sequoia`
      will apply the advice wherever the types match. Another example is when
      a `check` is applied to a range: the `tutor` is (recursively) passed on
      to the elements of the range and invoked wherever there's a match. In
      the case that there is no match, the `tutor` is silently ignored.

      Note that in the case where the advice function object provides a
      single binary overload of `operator()`, narrowing conversions are
      forbidden. However, if there are multiple binary overloads, narrowing
      conversions may occur.

      One pitfall to be aware of is that directly checking the result of a
      comparison boils down to checking two `bool`s. Thus, in this case,
      <pre>
      check(
        "x is assumed to be less than y",
        x < y,
        tutor{
          [](float a, float b) {
            return std::format("x is {}, y is {}", a, b);
          }
        }
      );
      </pre>
      the advice will be silently ignored. One fix for this is to instead
      construct the `tutor` from a function object with the signature
      `std::string operator()(bool, bool)`, which captures the values it
      reports:
      <pre>
      [&x, &y](bool, bool) { return std::format("x is {}, y is {}", x, y); }
      </pre>

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

  template<class Tutor>
  inline constexpr bool is_teacher_v{
    requires { typename std::remove_cvref_t<Tutor>::sequoia_advisor_type; }
  };

  template<class... U>
  struct ends_with_tutor : std::false_type
  {};

  template<class... U>
    requires    (sizeof...(U) > 0u)
             && is_teacher_v<decltype(std::get<sizeof...(U) - 1>(std::declval<std::tuple<std::remove_cvref_t<U>&...>>()))>
  struct ends_with_tutor<U...> : std::true_type
  {};


  template<class... U>
  inline constexpr bool ends_with_tutor_v{ends_with_tutor<U...>::value};

  /// \brief Helper for generating advice strings
  class advice_data
  {
  public:
    template<class Advisor, class T>
    advice_data(const tutor<Advisor>&, const T&, const T&)
    {}

    template<class Advisor, class T>
      requires std::is_invocable_r_v<std::string, tutor<Advisor>, T, T>
    advice_data(const tutor<Advisor>& advisor, const T& value, const T& prediction)
    {
        m_Advice = advisor(value, prediction);
        m_Prefix = advisor.prefix();
    }

    std::string& append_to(std::string& message) const;
  private:
    std::string m_Advice{}, m_Prefix{};
  };

  std::string& append_advice(std::string& message, const advice_data& adviceData);
}
