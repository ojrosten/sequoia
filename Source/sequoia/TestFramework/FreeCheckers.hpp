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

#include <format>

namespace sequoia::testing
{

  /*! \brief class template, specializations of which implement various comparisons for the specified type.
      \anchor value_tester_primary
   */
  template<class T> struct value_tester;

  //=========================== Types used to generate overload sets ===========================//

  struct equality_check_t {};
  struct simple_equality_check_t {};

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

  [[nodiscard]]
  inline std::string to_string(with_best_available_check_t)
  {
    return "with best available";
  }

  //=========================== Values defined for convenience ===========================//

  inline constexpr equality_check_t equality{};  
  inline constexpr simple_equality_check_t simple_equality{};
  inline constexpr equivalence_check_t equivalence{};
  inline constexpr weak_equivalence_check_t weak_equivalence{};
  inline constexpr with_best_available_check_t with_best_available{};

  template<class T>
  inline constexpr bool is_elementary_check{
       std::is_same_v<std::remove_cvref_t<T>, equality_check_t>
    || std::is_same_v<std::remove_cvref_t<T>, simple_equality_check_t>
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

  template<class Compare, class T>
  inline constexpr bool potentially_comparison_type{
    (std::invocable<Compare, T, T> || faithful_range<T>) && !(is_elementary_check<Compare> || is_customized_check<Compare>)
  };

  template<class T>
  inline constexpr bool has_fallback { requires { typename T::fallback; } };

  template<class CheckType, test_mode Mode, class T, class U, class... Args>
  concept tester_for = requires(test_logger<Mode>& logger, T&& obtained, const U& predicted, Args&&... args) {
    value_tester<std::remove_cvref_t<T>>::test(std::declval<CheckType>(),
                                               logger,
                                               std::forward<T>(obtained),
                                               predicted,
                                               std::forward<Args>(args)...);
  };

  template<class CheckType, test_mode Mode, class T, class U, class Advisor>
  concept binary_tester_for =
    tester_for<CheckType, Mode, T, U, tutor<Advisor>> || tester_for<CheckType, Mode, T, U>;

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

  template<test_mode Mode, class CheckType, class T, class U, class... Args>
  inline constexpr bool implements_general_equivalence_check{
       tester_for<CheckType, Mode, T, U, Args...>
    || tester_for<CheckType, Mode, T, U>
    || tester_for<CheckType, Mode, T, U, tutor<null_advisor>>
  };

  /*! \brief generic function that generates a check from any class providing a static check method.

      This employs a \ref test_logger_primary "sentinel" and so can be used naively.
   */

  template<class CheckType, test_mode Mode, class T, class U, class... Args>
    requires implements_general_equivalence_check<Mode, CheckType, T, U, Args...>
  bool general_equivalence_check(CheckType flavour,
                                 std::string description,
                                 test_logger<Mode>& logger,
                                 const T& obtained,
                                 const U& predicted,
                                 [[maybe_unused]] const Args&... args)
  {
    const auto msg{
      [flavour, &description] () -> std::string&& {
        return std::move(
                append_lines(description,
                             "Comparison performed using:",
                             make_type_info<value_tester<T>>(),
                             std::format("Checking for {} with:", to_string(flavour)), make_type_info<U>()).append("\n")
                );
      }
    };

    sentinel<Mode> sentry{logger, msg()};

    if constexpr(tester_for<CheckType, Mode, T, U, Args...>)
    {
      value_tester<T>::test(flavour, logger, obtained, predicted, args...);
    }
    else if constexpr(tester_for<CheckType, Mode, T, U>)
    {
      value_tester<T>::test(flavour, logger, obtained, predicted);
    }
    else if constexpr(tester_for<CheckType, Mode, T, U, tutor<null_advisor>>)
    {
      value_tester<T>::test(flavour, logger, obtained, predicted, tutor<null_advisor>{});
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
      std::string message{failure_reporter<Compare>::reporter(final_message_constant<IsFinalMessage>{}, compare, obtained, prediction)};
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


  template<class CheckType, test_mode Mode, class T, class U, class Advisor>
    requires binary_tester_for<CheckType, Mode, T, U, tutor<Advisor>>
  void select_test(CheckType flavour, test_logger<Mode>& logger, const T& obtained, const U& prediction, [[maybe_unused]] tutor<Advisor> advisor)
  {
    if constexpr(tester_for<CheckType, Mode, T, U, tutor<Advisor>>)
    {
      value_tester<T>::test(flavour, logger, obtained, prediction, advisor);
    }
    else if constexpr(tester_for<CheckType, Mode, T, U>)
    {
      value_tester<T>::test(flavour, logger, obtained, prediction);
    }
    else
    {
      static_assert(dependent_false<CheckType>::value, "Should never be triggered; indicates mismatch between requirement and logic");
    }
  }

  template<test_mode Mode, class T, class U, class Advisor>
  inline constexpr bool has_detailed_agnostic_check{
       binary_tester_for<equality_check_t, Mode, T, U, Advisor>
    || binary_tester_for<equivalence_check_t, Mode, T, U, Advisor>
    || binary_tester_for<weak_equivalence_check_t, Mode, T, U, Advisor>
    || binary_tester_for<with_best_available_check_t, Mode, T, U, Advisor>
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
  bool check_exception_thrown(std::string description, test_logger<Mode>& logger, Fn&& function, Postprocessor postprocessor={})
  {
    auto message{
      [&description]() -> std::string&& {
        return std::move(append_lines(description, "Expected Exception Type:", make_type_info<E>()));
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
             std::string description,
             test_logger<Mode>& logger,
             Iter first,
             Sentinel last,
             PredictionIter predictionFirst,
             PredictionSentinel predictionLast,
             tutor<Advisor> advisor = {})
  {
    auto info{
      [&description]() -> std::string&& {
        return description.empty() || description.back() == '\n' ? std::move(description) : std::move(description.append("\n"));
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
        auto mess{std::format("Element {} of range incorrect", dist)};
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
    requires potentially_comparison_type<Compare, T>
  bool check(Compare compare,
             std::string description,
             test_logger<Mode>& logger,
             const T& obtained,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description))};

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
               || binary_tester_for<equality_check_t, Mode, T, T, tutor<Advisor>>
               || faithful_range<T>)
  bool check(equality_check_t,
             std::string description,
             test_logger<Mode>& logger,
             const T& obtained,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description))};

    if constexpr(deep_equality_comparable<T>)
    {
      using finality = final_message_constant<!(binary_tester_for<equality_check_t, Mode, T, T, tutor<Advisor>> || faithful_range<T>)>;
      binary_comparison(finality{}, sentry, std::ranges::equal_to{}, obtained, prediction, advisor);
    }

    if constexpr(binary_tester_for<equality_check_t, Mode, T, T, tutor<Advisor>>)
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

  template<class CheckType, test_mode Mode, class T, class U, class... Args>
  requires has_generalized_equivalence_check<CheckType, Mode, T, U, Args...>
  bool check(CheckType flavour, std::string description, test_logger<Mode>& logger, const T& obtained, const U& prediction, Args&&... args)
  {
    if constexpr(implements_general_equivalence_check<Mode, CheckType, T, U, Args...>)
    {
      return general_equivalence_check(flavour,
                                       std::move(description),
                                       logger,
                                       obtained,
                                       prediction,
                                       std::forward<Args>(args)...);
    }
    else if constexpr(faithful_range<T>)
    {
      return check(flavour,
                   add_type_info<T>(std::move(description)),
                   logger,
                   std::begin(obtained),
                   std::end(obtained),
                   std::begin(prediction),
                   std::end(prediction),
                   std::forward<Args>(args)...);
    }
    else if constexpr(has_fallback<CheckType>)
    {
      using fallback = typename CheckType::fallback;
      return check(fallback{},
                   std::move(description),
                   logger,
                   obtained,
                   prediction,
                   std::forward<Args>(args)...);
    }
    else
    {
      static_assert(dependent_false<CheckType>::value, "Should never be triggered; indicates mismatch between requirement and logic");
    }
  }

  

  /*! \brief The workhorse for checking simple equality. */

  template<test_mode Mode, class T, class Advisor=null_advisor>
    requires (deep_equality_comparable<T> || faithful_range<T>)
  bool check(simple_equality_check_t,
             std::string description,
             test_logger<Mode>& logger,
             const T& obtained,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description))};

    using finality = final_message_constant<!faithful_range<T>>;
    binary_comparison(finality{}, sentry, std::ranges::equal_to{}, obtained, prediction, advisor);

    if constexpr(faithful_range<T>)
    {
      check(simple_equality, "", logger, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
    }

    return !sentry.failure_detected();
  }
  
  
  /*! \brief The workhorse for dispatching to the strongest available type of check. */

  template<test_mode Mode, class T, class U, class Advisor=null_advisor>
    requires (deep_equality_comparable<T> || has_detailed_agnostic_check<Mode, T, U, Advisor> || faithful_range<T>)
  bool check(with_best_available_check_t,
             std::string description,
             test_logger<Mode>& logger,
             const T& obtained,
             const U& prediction,
             tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(std::move(description))};

    // TO DO: generalize this to deep_equality_comparable_with<T, U>
    if constexpr(std::is_same_v<T, U> && deep_equality_comparable<T>)
    {
      using finality = final_message_constant<!(has_detailed_agnostic_check<Mode, T, U, Advisor> || faithful_range<T>)>;
      binary_comparison(finality{}, sentry, std::ranges::equal_to{}, obtained, prediction, advisor);
    }

    if constexpr(binary_tester_for<with_best_available_check_t, Mode, T, U, tutor<Advisor>>)
    {
      // TO DO: output could perhaps harmonize with general_equivalence_check
      select_test(with_best_available, logger, obtained, prediction, advisor);
    }
    else if constexpr(binary_tester_for<equality_check_t, Mode, T, U, tutor<Advisor>>)
    {
      select_test(equality_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr(binary_tester_for<equivalence_check_t, Mode, T, U, tutor<Advisor>>)
    {
      general_equivalence_check(equivalence, "", logger, obtained, prediction, advisor);
    }
    else if constexpr(binary_tester_for<weak_equivalence_check_t, Mode, T, U, tutor<Advisor>>)
    {
      general_equivalence_check(weak_equivalence, "", logger, obtained, prediction, advisor);
    }
    else if constexpr(faithful_range<T>)
    {
      check(with_best_available, "", logger, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
    }

    return !sentry.failure_detected();
  }

  template<test_mode Mode, class Advisor=null_advisor>
  bool check(std::string description, test_logger<Mode>& logger, const bool obtained, tutor<Advisor> advisor={})
  {
    return check(equality, std::move(description), logger, obtained, true, std::move(advisor));
  }

  /*! \brief Exposes elementary check methods, with the option to plug in arbitrary Extenders to compose functionality.

      This class template is templated on the enum class test_mode, with an Extender which will become
      variadic as soon as variadic friends are available.

      In its unextended form, the class is appropriate for plugging into basic_test to generate a base class
      appropriate for testing free functions. Within the unit test framework various Extenders are defined.
      For example, there are extensions to test types with regular semantics, types with move-only semantics, to do
      performance tests, and more, besides. The template design allows extenders to be conveniently mixed and
      matched via using declarations.

      \anchor checker_primary
   */

  template<test_mode Mode, class Extender>
  class checker : public Extender
  {
    friend Extender;
  public:
    constexpr static test_mode mode{Mode};
    using logger_type = test_logger<Mode>;

    checker() = default;

    checker(const checker&)            = delete;
    checker& operator=(const checker&) = delete;

    template<class T, class Advisor = null_advisor, class Self>
    bool check(this Self& self, equality_check_t, const reporter& description, const T& obtained, const T& prediction, tutor<Advisor> advisor = {})
    {
        return testing::check(equality, self.report(description), self.m_Logger, obtained, prediction, std::move(advisor));
    }

    template<class T, class U, class Advisor = null_advisor, class Self>
      requires (deep_equality_comparable<T> || has_detailed_agnostic_check<Mode, T, U, Advisor> || faithful_range<T>)
    bool check(this Self& self, with_best_available_check_t, const reporter& description, const T& obtained, const U& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(with_best_available, self.report(description), self.m_Logger, obtained, prediction, std::move(advisor));
    }

    template<class ValueBasedCustomizer, class T, class U, class Advisor = null_advisor, class Self>
    bool check(this Self& self, general_equivalence_check_t<ValueBasedCustomizer> checker, const reporter& description, const T& obtained, const U& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(checker, self.report(description), self.m_Logger, obtained, prediction, std::move(advisor));
    }

    template<class ValueBasedCustomizer, class T, class U, class Advisor = null_advisor, class Self>
    bool check(this Self& self, general_weak_equivalence_check_t<ValueBasedCustomizer> checker, const reporter& description, const T& obtained, const U& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(checker, self.report(description), self.m_Logger, obtained, prediction, std::move(advisor));
    }

    template<class Compare, class T, class Advisor = null_advisor, class Self>
      requires potentially_comparison_type<Compare, T>
    bool check(this Self& self, Compare compare, const reporter& description, const T& obtained, const T& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(std::move(compare), self.report(description), self.m_Logger, obtained, prediction, std::move(advisor));
    }

    template<class Advisor=null_advisor, class Self>
    bool check(this Self& self, const reporter& description, const bool obtained, tutor<Advisor> advisor={})
    {
      return testing::check(self.report(description), self.m_Logger, obtained, std::move(advisor));
    }

    template
    <
      class E,
      class Fn,
      invocable_r<std::string, std::string> Postprocessor=default_exception_message_postprocessor,
      class Self
    >
    bool check_exception_thrown(this Self& self, const reporter& description, Fn&& function, Postprocessor postprocessor={})
    {
      return testing::check_exception_thrown<E>(self.report(description), self.m_Logger, std::forward<Fn>(function), std::move(postprocessor));
    }

    template
    <
      std::input_or_output_iterator Iter,
      std::sentinel_for<Iter> Sentinel,
      std::input_or_output_iterator PredictionIter,
      std::sentinel_for<PredictionIter> PredictionSentinel,
      class Advisor=null_advisor,
      class Self
    >
    bool check(this Self& self,
               equality_check_t,
               const reporter& description,
               Iter first,
               Sentinel last,
               PredictionIter predictionFirst,
               PredictionSentinel predictionLast,
               tutor<Advisor> advisor={})
    {
      return testing::check(equality, self.report(description), self.m_Logger, first, last, predictionFirst, predictionLast, std::move(advisor));
    }

    template
    <
      class Compare,
      std::input_or_output_iterator Iter,
      std::sentinel_for<Iter> Sentinel,
      std::input_or_output_iterator PredictionIter,
      std::sentinel_for<PredictionIter> PredictionSentinel,
      class Advisor = null_advisor,
      class Self
    >
      requires potentially_comparison_type<Compare, typename Iter::value_type>
    bool check(this Self& self,
               Compare compare,
               const reporter& description,
               Iter first,
               Sentinel last,
               PredictionIter predictionFirst,
               PredictionSentinel predictionLast,
               tutor<Advisor> advisor={})
    {
      return testing::check(std::move(compare), self.report(description), self.m_Logger, first, last, predictionFirst, predictionLast, std::move(advisor));
    }

    template<class Stream>
    friend Stream& operator<<(Stream& os, const checker& c)
    {
      os << c.m_Logger;
      return os;
    }

    [[nodiscard]]
    log_summary summary(std::string_view prefix, const log_summary::duration delta) const
    {
      return log_summary{prefix, m_Logger, delta};
    }

    void reset_results() noexcept { m_Logger.reset_results(); }

    [[nodiscard]]
    bool has_critical_failures() const noexcept
    {
      return m_Logger.results().critical_failures > 0;
    }
  protected:
    explicit checker(active_recovery_files recovery)
      : m_Logger{std::move(recovery)}
    {}

    checker(checker&&)            noexcept = default;
    checker& operator=(checker&&) noexcept = default;

    ~checker() = default;

    [[nodiscard]]
    std::size_t checks() const noexcept { return m_Logger.checks(); }

    [[nodiscard]]
    std::size_t failures() const noexcept { return m_Logger.failures(); }

    [[nodiscard]]
    const uncaught_exception_info& exceptions_detected_by_sentinel() const noexcept
    {
      return m_Logger.exceptions_detected_by_sentinel();
    }

    [[nodiscard]]
    sentinel<Mode> make_sentinel(std::string message)
    {
      return {m_Logger, std::move(message)};
    }

    [[nodiscard]]
    std::string_view top_level_message() const
    {
      return m_Logger.top_level_message();
    }

    [[nodiscard]]
    const failure_output& failure_messages() const noexcept
    {
      return m_Logger.results().failure_messages;
    }
  private:
    test_logger<Mode> m_Logger;
  };
}
