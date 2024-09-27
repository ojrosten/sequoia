////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Free functions for performing checks, together with the 'checker' class template which wraps them.

    Given a type, `T`, any reasonable testing framework must provide a mechanism for checking whether or
    not two instances of `T` are, in some sense, the same. If the type implements `operator==` then it
    is natural to utilize this. However, there is much more to the story. First of all, if this check
    fails then, in order to be useful, there must be some way of serializing the state of `T`. This may
    be done by specializing the class template \ref serializer_primary "serializer" for cases where
    `operator<<` is not appropriately overloaded.

    However, there is an alternative which may be superior. Consider trying to implement `vector`.
    This class has various const accessors suggesting that, if `operator==` returns `false`, then the
    accessors can be used to probe the exact nature of the inequality. To this end, a class template
    \ref value_tester_primary "value_tester" is defined. Specializations that provide an implementation
    for the static method `void test(equality_check_t, ...)` perform a detailed check of the equality
    of the state of two supposedly equal instance of a class. Note that there is no need for the
    value_tester to include checks of `operator==` or `operator!=`, since these will be done,
    regardless. Indeed, the value_tester will be triggered independently of whether `operator==`
    fails since either the latter or the accessors may have bugs, and introducing unnecessary dependencies
    would reduce the fidelity of the testing framework.

    If a value_tester is supplied, clients may typically expect a notification that `operator==` has returned
    `false`. There will then be a series of
    subsequent checks which will reveal the precise nature of the failure. For example, for vectors, one
    will be told whether the sizes differ and, if not, the element which is causing the difference
    between the two supposedly equal instances. If the vector holds a user-defined type then, so long as
    this has its own value_tester, the process will continue until a type is reached without
    a value_tester; typically this will be a sufficiently simple type that serialization is
    the appropriate solution (as is the case for may built-in types).

    Suppose a client wishes to compare instances of `some_container<T>`. If `some_container<T>` has a specialization
    of \ref value_tester_primary "value_tester" then this will be used; if it does
    not then reflection is used to determine if `some_container` overloads `begin` and `end`.
    If so, then it is treated as a range and
    all that is required is to implement a `value_tester` for `T` (unless serialization is prefered).
    If `some_container` is user-defined, it is wisest to provide an overload of the
    \ref value_tester_primary "value_tester".
    However, if the container is part of std, it is probably safe to assume it works correctly and so
    instead effort can be directed focused on T.

    With this in mind, imagine creating a container, `C`. One of the first things one may wish to do is to check
    that it is correctly initialized. It would be a mistake to use `value_tester<C>::test(equality_check_t, ...)` for this
    since, to do so, a second, identical instance would need to be created. This is circular and prone to
    false positives. However, one may instead furnish `value_tester<C>` with methods
    `test(equivalence_check_t, ...)` and/or `test(weak_equivalence_check_t, ...)`.
    
    We may consider a value for e.g. `std::vector<int>` to be equivalent to an 
    `initializer_list<int>` in the sense that they hold (at
    the relevant point of the program) the same elements. Thus, an appropriate defintion of
    `test(equivalence_check_t, ...)`
    is supplied. Of course, there is more to a vector than the
    values it holds: there is the entire allocator framework too. In this case, however, it is not part of
    the logical structure and, indeed, the state of the allocator is not considered in `vector<T>::operator==`.
    Thus it is philosophically reasonable to consider equivalence of `vector` and `initializer_list`. However,
    sometimes is is useful to check the equivalence of the state of an instance of `T` to a proper subset of
    the logical state of an instance of some S. For this purpose, an appropriate defintion of
    `test(weak_equivalence_check_t, ...)`.
*/

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TestFramework/BinaryRelationships.hpp"
#include "sequoia/TestFramework/Advice.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/Core/Meta/Utilities.hpp"

namespace sequoia::testing
{

  /*! \brief class template, specializations of which implement various comparisons for the specified type.
      \anchor value_tester_primary
   */
  template<class T> struct value_tester;

  //=========================== Types used to generate overload sets ===========================//

  struct equality_check_t {};

  template<class ValueBasedCustomizer>
  struct general_equivalence_check_t
  {
    using customizer_type = ValueBasedCustomizer;

    template<class Customizer>
    using rebind_check_type = general_equivalence_check_t<Customizer>;

    customizer_type customizer;
  };

  template<class ValueBasedCustomizer>
    requires std::is_void_v<ValueBasedCustomizer>
  struct general_equivalence_check_t<ValueBasedCustomizer>
  {
    using fallback = equality_check_t;
  };

  using equivalence_check_t = general_equivalence_check_t<void>;

  template<class ValueBasedCustomizer>
  struct general_weak_equivalence_check_t
  {
    using customizer_type = ValueBasedCustomizer;

    template<class Customizer>
    using rebind_check_type = general_weak_equivalence_check_t<Customizer>;

    customizer_type customizer;
  };

  template<class ValueBasedCustomizer>
    requires std::is_void_v<ValueBasedCustomizer>
  struct general_weak_equivalence_check_t<ValueBasedCustomizer>
  {
    using fallback = general_equivalence_check_t<ValueBasedCustomizer>;
  };

  using weak_equivalence_check_t = general_weak_equivalence_check_t<void>;

  struct with_best_available_check_t {};

  template<class ValueBasedCustomizer>
  [[nodiscard]]
  std::string to_string(general_equivalence_check_t<ValueBasedCustomizer>)
  {
    return "equivalence";
  }

  template<class ValueBasedCustomizer>
  [[nodiscard]]
  std::string to_string(general_weak_equivalence_check_t<ValueBasedCustomizer>)
  {
    return "weak equivalence";
  }

  //=========================== Values defined for convenience ===========================//

  inline constexpr equality_check_t equality{};
  inline constexpr equivalence_check_t equivalence{};
  inline constexpr weak_equivalence_check_t weak_equivalence{};
  inline constexpr with_best_available_check_t with_best_available{};

  template<class T>
  inline constexpr bool is_elementary_check{
       std::is_same_v<std::remove_cvref_t<T>, equality_check_t>
    || std::is_same_v<std::remove_cvref_t<T>, equivalence_check_t>
    || std::is_same_v<std::remove_cvref_t<T>, weak_equivalence_check_t>
    || std::is_same_v<std::remove_cvref_t<T>, with_best_available_check_t>
  };

  template<class T>
  inline constexpr bool is_customized_check{
    requires {
      typename T::customizer_type;
      typename T::template rebind_check_type<typename T::customizer_type>;
      requires std::is_same_v<typename T::template rebind_check_type<typename T::customizer_type>, T>;
    }
  };

  static_assert(is_customized_check<general_equivalence_check_t<int>>);
  static_assert(is_customized_check<general_weak_equivalence_check_t<int>>);

  template<class Compare, class T>
  inline constexpr bool maybe_comparison_type{
    (std::invocable<Compare, T, T> || faithful_range<T>) && !(is_elementary_check<Compare> || is_customized_check<Compare>)
  };

  template<class T>
  inline constexpr bool has_fallback {
    requires { typename T::fallback; }
  };

  template<class CheckType, test_mode Mode, class T, class... Args>
  concept tester_for = requires(test_logger<Mode>& logger, Args&&... args) {
    value_tester<T>::test(std::declval<CheckType>(), logger, std::forward<Args>(args)...);
  };

  template<class CheckType, test_mode Mode, class T, class Advisor>
  concept binary_tester_for =
    tester_for<CheckType, Mode, T, T, T, tutor<Advisor>> || tester_for<CheckType, Mode, T, T, T>;

  template<class... Ts>
  struct equivalent_type_processor
  {
    constexpr static bool ends_with_tutor{false};

    [[nodiscard]]
    static std::string info()
    {
      return make_type_info<Ts...>();
    }
  };

  template<class... Ts>
    requires ends_with_tutor_v<Ts...>
  struct equivalent_type_processor<Ts...>
  {
    constexpr static bool ends_with_tutor{true};

    [[nodiscard]]
    static std::string info()
    {
      return info(std::make_index_sequence<sizeof...(Ts) - 1>{});
    }
  private:
    template<std::size_t... I>
    [[nodiscard]]
    static std::string info(std::index_sequence<I...>)
    {
      return make_type_info<std::remove_cvref_t<decltype(std::get<I>(std::declval<std::tuple<Ts...>>()))>...>();
    }
  };


  /*! \brief class template, specializations of which extract messages from various exception types.
     \anchor exception_message_extractor_primary
  */
  template<class>
  struct exception_message_extractor;

  template<class E>
  requires std::derived_from<E, std::exception>
    struct exception_message_extractor<E>
  {
    [[nodiscard]]
    static std::string get(const E& e) { return e.what(); }
  };

  template<class E>
    requires (!std::derived_from<E, std::exception>) && serializable<E>
  struct exception_message_extractor<E>
  {
    [[nodiscard]]
    static std::string get(const E& e) { return to_string(e); }
  };

  template<class T>
  struct equivalence_checker_delegator
  {
    template<class CheckType, test_mode Mode, class... Args>
      requires tester_for<CheckType, Mode, T, Args...>
    void operator()(CheckType flavour, test_logger<Mode>& logger, Args&&... args) const
    {
      value_tester<T>::test(flavour, logger, std::forward<Args>(args)...);
    }
  };

  namespace impl
  {
    // TO DO: investigate if this 'concept' can be traded for a constexpr bool
    template<class Fn, class... Args>
    concept invocable_without_last_arg = (sizeof...(Args) > 0) && requires(Fn && fn, Args&&... args) {
      invoke_with_specified_args(fn, std::make_index_sequence<sizeof...(Args) - 1>(), std::forward<Args>(args)...);
    };
  }

  template<test_mode Mode, class CheckType, class T, class S, class... U>
  inline constexpr bool implements_general_equivalence_check{
            tester_for<CheckType, Mode, T, T, S, U...>
    || (    equivalent_type_processor<S, U...>::ends_with_tutor
         && impl::invocable_without_last_arg<equivalence_checker_delegator<T>, CheckType, test_logger<Mode>&, T, S, U...>)
    ||      tester_for<CheckType, Mode, T, T, S, U..., tutor<null_advisor>>
  };

  /*! \brief generic function that generates a check from any class providing a static check method.

      This employs a \ref test_logger_primary "sentinel" and so can be used naively.
   */

  template<class CheckType, test_mode Mode, class T, class S, class... U>
    requires implements_general_equivalence_check<Mode, CheckType, T, S, U...>
  bool general_equivalence_check(CheckType flavour, std::string_view description, test_logger<Mode>& logger, const T& obtained, const S& s, const U&... u)
  {
    using processor = equivalent_type_processor<S, U...>;

    const auto msg{
      [flavour, description, types{processor::info()}] (){
        return append_lines(description,
                            "Comparison performed using:",
                            make_type_info<value_tester<T>>(),
                            std::string{"Checking for "}.append(to_string(flavour)).append(" with:"),
                            types)
               .append("\n");
      }()
    };

    sentinel<Mode> sentry{logger, msg};

    if constexpr(tester_for<CheckType, Mode, T, T, S, U...>)
    {
      value_tester<T>::test(flavour, logger,  obtained, s, u...);
    }
    else if constexpr(processor::ends_with_tutor)
    {
      using delegator = equivalence_checker_delegator<T>;

      if constexpr(impl::invocable_without_last_arg<delegator, CheckType, test_logger<Mode>&, T, S, U...>)
      {
        invoke_with_specified_args(delegator{}, std::make_index_sequence<sizeof...(U) + 3>(), flavour, logger, obtained, s, u...);
      }
      else
      {
        static_assert(dependent_false<value_tester<T>>::value, "Should never be triggered; indicates mismatch between requirement and logic");
      }
    }
    else if constexpr(tester_for<CheckType, Mode, T, T, S, U..., tutor<null_advisor>>)
    {
      value_tester<T>::test(flavour, logger, obtained, s, u..., tutor<null_advisor>{});
    }
    else
    {
      static_assert(dependent_false<value_tester<T>>::value, "Should never be triggered; indicates mismatch between requirement and logic");
    }

    return !sentry.failure_detected();
  }

  template<bool IsFinalMessage, test_mode Mode, class Compare, class T, class Advisor>
    requires (std::invocable<Compare, T, T> && (!IsFinalMessage || reportable<T>) && !std::is_array_v<T>)
  void binary_comparison(final_message_constant<IsFinalMessage>, sentinel<Mode>& sentry, Compare compare, const T& obtained, const T& prediction, tutor<Advisor> advisor)
  {
    sentry.log_check();
    if(!compare(obtained, prediction))
    {
      std::string message{failure_reporter<Compare>::report(final_message_constant<IsFinalMessage>{}, compare, obtained, prediction)};
      append_advice(message, {advisor, obtained, prediction});

      sentry.log_failure(message);
    }
  }

  template<class CheckType, test_mode Mode, class T, class S, class... U>
  inline constexpr bool has_generalized_equivalence_check{
      !(   std::is_same_v<CheckType, equality_check_t>
        || std::is_same_v<CheckType, with_best_available_check_t>)
    && (   implements_general_equivalence_check<Mode, CheckType, T, S, U...>
        || faithful_range<T>
        || has_fallback<CheckType>)
  };


  template<class CheckType, test_mode Mode, class T, class Advisor>
    requires binary_tester_for<CheckType, Mode, T, tutor<Advisor>>
  void select_test(CheckType flavour, test_logger<Mode>& logger, const T& obtained, const T& prediction, [[maybe_unused]] tutor<Advisor> advisor)
  {
    if constexpr(tester_for<CheckType, Mode, T, T, T, tutor<Advisor>>)
    {
      value_tester<T>::test(flavour, logger, obtained, prediction, advisor);
    }
    else if constexpr(tester_for<CheckType, Mode, T, T, T>)
    {
      value_tester<T>::test(flavour, logger, obtained, prediction);
    }
    else
    {
      static_assert(dependent_false<CheckType>::value, "Should never be triggered; indicates mismatch between requirement and logic");
    }
  }

  template<test_mode Mode, class T, class Advisor>
  inline constexpr bool has_detailed_agnostic_check{
       binary_tester_for<equality_check_t, Mode, T, Advisor>
    || binary_tester_for<equivalence_check_t, Mode, T, Advisor>
    || binary_tester_for<weak_equivalence_check_t, Mode, T, Advisor>
    || binary_tester_for<with_best_available_check_t, Mode, T, Advisor>
  };

  struct default_exception_message_postprocessor
  {
    [[nodiscard]]
    std::string operator()(std::string mess) const
    {
      return mess;
    }
  };

  template
  <
    class E,
    test_mode Mode,
    class Fn,
    invocable_r<std::string, std::string> Postprocessor=default_exception_message_postprocessor
  >
  bool check_exception_thrown(std::string_view description, test_logger<Mode>& logger, Fn&& function, Postprocessor postprocessor={})
  {
    auto message{
      [description](){
        return append_lines(description, "Expected Exception Type:", make_type_info<E>());
      }()
    };

    sentinel<Mode> sentry{logger, message};
    sentry.log_check();
    try
    {
      function();
      sentry.log_failure("No exception thrown");
      return false;
    }
    catch(const E& e)
    {
      sentry.log_caught_exception_message(postprocessor(exception_message_extractor<E>::get(e)));
      return true;
    }
    catch(const std::exception& e)
    {
      std::string msg{append_lines("Unexpected exception thrown (caught by std::exception&):", "\"").append(e.what()).append("\"\n")};

      sentry.log_failure(msg);
      return false;
    }
    catch(...)
    {
      sentry.log_failure("Unknown exception thrown\n");
      return false;
    }
  }

  //================= namespace-level convenience functions =================//

  /*! \brief The workhorse for comparing the contents of ranges.

  */

  template
  <
    class CheckType,
    test_mode Mode,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel,
    class Advisor = null_advisor
  >
  bool check(CheckType flavour,
             std::string_view description,
             test_logger<Mode>& logger,
             Iter first,
             Sentinel last,
             PredictionIter predictionFirst,
             PredictionSentinel predictionLast,
             tutor<Advisor> advisor = {})
  {
    auto info{
      [description]() {
        return description.empty() || description.back() == '\n'
          ? std::string{description} : std::string{description}.append("\n");
      }
    };

    sentinel<Mode> sentry{logger, info()};
    bool equal{true};

    const auto actualSize{fixed_width_unsigned_cast(std::ranges::distance(first, last))};
    const auto predictedSize{fixed_width_unsigned_cast(std::ranges::distance(predictionFirst, predictionLast))};
    if(check(equality, "Container size wrong", logger, actualSize, predictedSize))
    {
      auto predictionIter{predictionFirst};
      auto iter{first};
      for(; predictionIter != predictionLast; std::ranges::advance(predictionIter, 1), std::ranges::advance(iter, 1))
      {
        const auto dist{std::ranges::distance(predictionFirst, predictionIter)};
        auto mess{("Element " + std::to_string(dist)).append(" of range incorrect")};
        if(!check(flavour, std::move(mess), logger, *iter, *predictionIter, advisor)) equal = false;
      }
    }
    else
    {
      equal = false;
    }

    return equal;
  }

  /*! \brief The workhorse for performing a check with respect to a user-specified binary operator 
  
      The comparison object either implements binary comparison for T or it is fed into the overload for ranges.
   */

  template<class Compare, test_mode Mode, class T, class Advisor=null_advisor>
    requires maybe_comparison_type<Compare, T>
  bool check(Compare compare,
             std::string_view description,
             test_logger<Mode>& logger,
             const T& obtained,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(std::invocable<Compare, T, T>)
    {
      using finality = final_message_constant<!faithful_range<T>>;
      binary_comparison(finality{}, sentry, std::move(compare), obtained, prediction, advisor);
    }
    else
    {
      check(std::move(compare), "", logger, obtained.begin(), obtained.end(), prediction.begin(), prediction.end(), advisor);
    }

    return !sentry.failure_detected();
  }

  /*! \brief The workhorse of equality checking, which takes responsibility for reflecting upon types
      and then dispatching, appropriately.

      The input type, T, must either

       1. be equality comparable and/or possess a value_tester;
       2. be a range.

      If this constraint is not satisfied there are several possible remedies:

        1. Provide a specialization of value_tester or ensure operator== and !=
           exist, together with a specialization of serializer.

        2. If the invocation arises from a fallback from a failed attempt to invoke 
           either `check(equivalence_check_t,...)` or `check(weak_equivalence_check_t,...)`
           then consider furnishing either of these with an implementation.
   */

  template<test_mode Mode, class T, class Advisor=null_advisor>
    requires (    deep_equality_comparable<T>
               || binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>>
               || faithful_range<T>)
  bool check(equality_check_t,
             std::string_view description,
             test_logger<Mode>& logger,
             const T& obtained,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(deep_equality_comparable<T>)
    {
      using finality = final_message_constant<!(binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>> || faithful_range<T>)>;
      binary_comparison(finality{}, sentry, std::ranges::equal_to{}, obtained, prediction, advisor);
    }

    if constexpr(binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(equality_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr(faithful_range<T>)
    {
      check(equality, "", logger, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
    }

    return !sentry.failure_detected();
  }

  /*! \brief The workhorse for (weak) equivalence checking

      This function will reflect on whether an appropriate invocation of 
      `check(equivalence_check_t,...)` or `check(weak_equivalence_check_t,...)` exists.
      If so, it will be used and if not the method will attempt to interpret T as a range.
      If both of these fail then:
      
      1. A weak equivalence check will attempt to fall back to an equivalence check;
      2. An equivalence check will attemp to fall back to a equality check.

   */

  template<class CheckType, test_mode Mode, class T, class S, class... U>
    requires has_generalized_equivalence_check<CheckType, Mode, T, S, U...>
  bool check(CheckType flavour, std::string_view description, test_logger<Mode>& logger, const T& obtained, S&& s, U&&... u)
  {
    if constexpr(implements_general_equivalence_check<Mode, CheckType, T, S, U...>)
    {
      return general_equivalence_check(flavour, description, logger, obtained, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(faithful_range<T>)
    {
      return check(flavour, add_type_info<T>(description), logger, std::begin(obtained), std::end(obtained), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)), std::forward<U>(u)...);
    }
    else if constexpr(has_fallback<CheckType>)
    {
      using fallback = typename CheckType::fallback;
      return check(fallback{}, description, logger, obtained, std::forward<S>(s), std::forward<U>(u)...);
    }
    else
    {
      static_assert(dependent_false<CheckType>::value, "Should never be triggered; indicates mismatch between requirement and logic");
    }
  }

  /*! \brief The workhorse for dispatching to the strongest available type of check.
  
   */

  template<test_mode Mode, class T, class Advisor=null_advisor>
    requires (deep_equality_comparable<T> || has_detailed_agnostic_check<Mode, T, Advisor> || faithful_range<T>)
  bool check(with_best_available_check_t,
             std::string_view description,
             test_logger<Mode>& logger,
             const T& obtained,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(deep_equality_comparable<T>)
    {
      using finality = final_message_constant<!(has_detailed_agnostic_check<Mode, T, Advisor> || faithful_range<T>)>;
      binary_comparison(finality{}, sentry, std::ranges::equal_to{}, obtained, prediction, advisor);
    }

    if constexpr(binary_tester_for<with_best_available_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(with_best_available_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr(binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(equality_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr(binary_tester_for<equivalence_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(equivalence_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr(binary_tester_for<weak_equivalence_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(weak_equivalence_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr(faithful_range<T>)
    {
      check(with_best_available_check_t{}, "", logger, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
    }

    return !sentry.failure_detected();
  }

  template<test_mode Mode, class Advisor=null_advisor>
  bool check(std::string_view description, test_logger<Mode>& logger, const bool obtained, tutor<Advisor> advisor={})
  {
    return check(equality, description, logger, obtained, true, std::move(advisor));
  }

  /*! \brief Exposes elementary check methods, with the option to plug in arbitrary Extenders to compose functionality.

      This class template is templated on the enum class test_mode, together with a variadic set of Extenders.

      In its unextended form, the class is appropriate for plugging into basic_test to generate a base class
      appropriate for testing free functions. Within the unit test framework various Extenders are defined.
      For example, there are extensions to test types with regular semantics, types with move-only semantics, to do
      performance tests, and more, besides. The template design allows extenders to be conveniently mixed and
      matched via using declarations.

      Each extender must be initialized with a reference to the test_logger held by the checker.
      To ensure the correct order of initialization, the test_logger is inherited privately.

      \anchor checker_primary
   */

  template<class T, test_mode Mode>
  concept extender = (sizeof(T) == sizeof(void*)) && requires(test_logger<Mode>& logger) {
    new T{logger};
  };

  template<test_mode Mode, extender<Mode>... Extenders>
  class checker : private test_logger<Mode>, public Extenders...
  {
  public:
    constexpr static test_mode mode{Mode};
    using logger_type = test_logger<Mode>;

    checker() : Extenders{logger()}... {}

    checker(const checker&)            = delete;
    checker& operator=(const checker&) = delete;

    template<class T, class Advisor=null_advisor>
    bool check(equality_check_t, std::string_view description, const T& obtained, const T& prediction, tutor<Advisor> advisor={})
    {
      return testing::check(equality, description, logger(), obtained, prediction, std::move(advisor));
    }

    template<class Self, class T, class Advisor = null_advisor>
    bool check(this Self&& self, equality_check_t, const report& description, const T& obtained, const T& prediction, tutor<Advisor> advisor = {})
    {
        return testing::check(equality, self.report_line(description), self.logger(), obtained, prediction, std::move(advisor));
    }

    template<class T, class Advisor = null_advisor>
    bool check(with_best_available_check_t, std::string_view description, const T& obtained, const T& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(with_best_available, description, logger(), obtained, prediction, std::move(advisor));
    }

    template<class ValueBasedCustomizer, class T, class S, class... U>
    bool check(general_equivalence_check_t<ValueBasedCustomizer> checker, std::string_view description, const T& obtained, S&& s, U&&... u)
    {
      return testing::check(checker, description, logger(), obtained, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class ValueBasedCustomizer, class T, class S, class... U>
    bool check(general_weak_equivalence_check_t<ValueBasedCustomizer> checker, std::string_view description, const T& obtained, S&& s, U&&... u)
    {
      return testing::check(checker, description, logger(), obtained, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class Compare, class T, class Advisor = null_advisor>
      requires maybe_comparison_type<Compare, T>
    bool check(Compare compare, std::string_view description, const T& obtained, const T& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(std::move(compare), description, logger(), obtained, prediction, std::move(advisor));
    }

    template<class Advisor=null_advisor>
    bool check(std::string_view description, const bool obtained, tutor<Advisor> advisor={})
    {
      return testing::check(description, logger(), obtained, std::move(advisor));
    }

    template
    <
      class E,
      class Fn,
      invocable_r<std::string, std::string> Postprocessor=default_exception_message_postprocessor
    >
    bool check_exception_thrown(std::string_view description, Fn&& function, Postprocessor postprocessor={})
    {
      return testing::check_exception_thrown<E>(description, logger(), std::forward<Fn>(function), std::move(postprocessor));
    }

    template
    <
      std::input_or_output_iterator Iter,
      std::sentinel_for<Iter> Sentinel,
      std::input_or_output_iterator PredictionIter,
      std::sentinel_for<PredictionIter> PredictionSentinel,
      class Advisor=null_advisor
    >
    bool check(equality_check_t,
               std::string_view description,
               Iter first,
               Sentinel last,
               PredictionIter predictionFirst,
               PredictionSentinel predictionLast,
               tutor<Advisor> advisor={})
    {
      return testing::check(equality, description, logger(), first, last, predictionFirst, predictionLast, std::move(advisor));
    }

    template
    <
      class Compare,
      std::input_or_output_iterator Iter,
      std::sentinel_for<Iter> Sentinel,
      std::input_or_output_iterator PredictionIter,
      std::sentinel_for<PredictionIter> PredictionSentinel,
      class Advisor = null_advisor
    >
      requires maybe_comparison_type<Compare, typename Iter::value_type>
    bool check(Compare compare,
               std::string_view description,
               Iter first,
               Sentinel last,
               PredictionIter predictionFirst,
               PredictionSentinel predictionLast,
               tutor<Advisor> advisor={})
    {
      return testing::check(std::move(compare), description, logger(), first, last, predictionFirst, predictionLast, std::move(advisor));
    }

    template<class Stream>
    friend Stream& operator<<(Stream& os, const checker& c)
    {
      os << c.logger();
      return os;
    }

    [[nodiscard]]
    log_summary summary(std::string_view prefix, const log_summary::duration delta) const
    {
      return log_summary{prefix, logger(), delta};
    }

    void reset_results() noexcept { logger().reset_results(); }

    [[nodiscard]]
    bool has_critical_failures() const noexcept
    {
      return logger().results().critical_failures > 0;
    }
  protected:
    checker(checker&& other) noexcept
      : logger_type{static_cast<logger_type&&>(other)}
      , Extenders{logger()}...
    {}

    checker& operator=(checker&& other) noexcept
    {
      if(&other != this)
      {
        static_cast<logger_type&>(*this) = static_cast<logger_type&&>(other);
        // The only state the Extenders possess is a handle to the logger,
        // which should not be reset.
      }

      return *this;
    }

    ~checker() = default;

    [[nodiscard]]
    std::size_t checks() const noexcept { return logger().checks(); }

    [[nodiscard]]
    std::size_t failures() const noexcept { return logger().failures(); }

    [[nodiscard]]
    const uncaught_exception_info& exceptions_detected_by_sentinel() const noexcept
    {
      return logger().exceptions_detected_by_sentinel();
    }

    [[nodiscard]]
    sentinel<Mode> make_sentinel(std::string_view message)
    {
      return {logger(), message};
    }

    [[nodiscard]]
    std::string_view top_level_message() const
    {
      return logger().top_level_message();
    }

    [[nodiscard]]
    const failure_output& failure_messages() const noexcept
    {
      return logger().results().failure_messages;
    }

    void recovery(active_recovery_files files)
    {
      test_logger<Mode>::recovery(std::move(files));
    }
  private:
    [[nodiscard]]
    test_logger<Mode>& logger() noexcept
    {
      return static_cast<test_logger<Mode>&>(*this);
    }

    [[nodiscard]]
    const test_logger<Mode>& logger() const noexcept
    {
      return static_cast<const test_logger<Mode>&>(*this);
    }
  };
}
