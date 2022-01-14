////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Free functions for performing checks, together with the 'checker' class template which wraps them.

    Given a type, T, any reasonable testing framework must provide a mechanism for checking whether or
    not two instances of T are, in some sense, the same. If the type implements operator== then it
    is natural to utilize this. However, there is much more to the story. First of all, if this check
    fails then, in order to be useful, there must be some way of serializing the state of T. This may
    be done by specializing the class template
    \ref string_maker_primary "string_maker" for cases where operator<< is not appropriately
    overloaded.

    However, there is an alternative which may be superior. Consider trying to implement vector.
    This class has various const accessors suggesting that, if operator== returns false, then the
    accessors can be used to probe the exact nature of the inequality. To this end, a class template
    \ref value_tester_primary "value_tester" is defined. Specializations
    perform a detailed check of the equality
    of the state of two supposedly equal instance of a class. Note that there is no need for the
    value_tester to include checks of operator== or operator!=, since these will be done,
    regardless. Indeed, the value_tester will be triggered independently of whether operator==
    fails since either the latter or the accessors may have bugs, and introducing unnecessary dependencies
    would reduce the fidelity of the testing framework.

    If a value_tester is supplied, then compile-time logic will ignore any attempt to
    serialize objects of type T. Typically, clients may expect a notification that operator== has returned
    false. There will then be a series of
    subsequent checks which will reveal the precise nature of the failure. For example, for vectors, one
    will be told whether the sizes differ and, if not, the element which is causing the difference
    between the two supposedly equal instances. If the vector holds a user-defined type then, so long as
    this has its own value_tester, the process will continue until a type is reached without
    a value_tester; typically this will be a sufficiently simple type that serialization is
    the appropriate solution (as is the case for may built-in types).

    Suppose a client wishes to compare instance of some_container<T>. If some_container has a specialization
    of \ref value_tester_primary "value_tester" then this will be used; if it does
    not then reflection is used to
    determine if some_container overloads begin and end. If so, then it is treated as a range and
    all that is required is to implement a value_tester for T (unless serialization is prefered).
    If some_container is user-defined, it is wisest to provide an overload of the
    \ref value_tester_primary "value_tester".
    However, if the container is part of std, it is probably safe to assume it works correctly and so
    instead effort can be directed focused on T.

    With this in mind, imagine creating a container. One of the first things one may wish to do is to check
    that it is correctly initialized. It would be a mistake to use the
    \ref value_tester_primary "value_tester" for this
    since, to do so, a second, identical instance would need to be created. This is circular and prone to
    false positives. Consequently, the testing framework also defines a pair of class templates that
    complement
    \ref value_tester_primary "value_tester":
    \ref equivalence_checker_primary "equivalence_checker" and
    \ref weak_equivalence_checker_primary "weak_equivalence_checker". We may
    consider a value for std::vector to be equivalent to an initializer_list in the sense that they hold (at
    the relevant point of the program) the same elements. Thus, a specialization
    equivalence_checker<vector, initializer_list> is supplied. Of course, there is more to a vector than the
    values it holds: there is the entire allocator framework too. In this case, however, it is not part of
    the logical structure and, indeed, the state of the allocator is not considered in vector::operator==.
    Thus it is philosophically reasonable to consider equivalence of vector and initializer_list. However,
    sometimes is is useful to check the equivalence of the state of an instance of T to a proper subset of
    the logical state of an instance of some S. For this purpose, the class template
    \ref weak_equivalence_checker_primary "weak_equivalence_checker" is supplied.

    Both the \ref equivalence_checker_primary "equivalence_checker" and its weak counterpart can be fed >=2
    template type arguments. While for
    a vector we would just feed in two types (vector and initializer_list), in some cases we may need more.
    For example, a graph has both nodes and edges and so a graph may be considered equivalent to two
    types representing these structures.
*/

#include "sequoia/TestFramework/CoreInfrastructure.hpp"
#include "sequoia/TestFramework/BinaryRelationships.hpp"
#include "sequoia/TestFramework/Advice.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/Core/Meta/Utilities.hpp"

namespace sequoia::testing
{
  template<class Tag, test_mode Mode, class T, class... Args>
  concept tester_for = requires(test_logger<Mode>& logger, Args&&... args) {
    value_tester<T>::test(Tag{}, logger, std::forward<Args>(args)...);
  };

  template<class Tag, test_mode Mode, class T, class Advisor>
  concept binary_tester_for = requires(test_logger<Mode>&logger, const T& value, const T& prediction, Advisor advisor) {
    requires (tester_for<Tag, Mode, T, T, T, tutor<Advisor>> || tester_for<Tag, Mode, T, T, T>);
  };

  template<class T>
  struct value_based_customization
  {
    constexpr static bool is_customizer_v{true};
    using value_type = T;

    T customizer;
  };

  template<>
  struct value_based_customization<void>
  {
    using value_type = void;
  };

  template<class T>
  inline constexpr bool is_value_customizer{
    requires { T::is_customizer_v; }
  };

  namespace impl
  {
    // TO DO: investigate if this 'concept' can be traded for a constexpr bool
    template<class Fn, class... Args>
    concept invocable_without_last_arg = (sizeof...(Args) > 0) && requires(Fn&& fn, Args&&... args) {
      invoke_with_specified_args(fn, std::make_index_sequence<sizeof...(Args) - 1>(), std::forward<Args>(args)...);
    };
  }

  template<class... Ts>
  struct equivalent_type_processor
  {
    static constexpr bool ends_with_tutor{};

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
    static constexpr bool ends_with_tutor{true};

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

  template<class Tag, class T>
    requires is_equivalence_disambiguator_v<Tag>
  struct equivalence_checker_delegator
  {
    template<test_mode Mode, class... Args>
      requires tester_for<Tag, Mode, T, Args...>
    void operator()(test_logger<Mode>& logger, Args&&... args) const
    {
      value_tester<T>::test(Tag{}, logger, std::forward<Args>(args)...);
    }
  };

  template<test_mode Mode, class Tag, class Customization, class T, class S, class... U>
  inline constexpr bool implements_general_equivalence_check{
    requires {
      requires (    tester_for<Tag, Mode, T, Customization, T, S, U...>
                 || tester_for<Tag, Mode, T, T, S, U...>
                 || (          equivalent_type_processor<S, U...>::ends_with_tutor
                       && (    impl::invocable_without_last_arg<equivalence_checker_delegator<Tag, T>, test_logger<Mode>&, Customization, T, S, U...>
                            || impl::invocable_without_last_arg<equivalence_checker_delegator<Tag, T>, test_logger<Mode>&, T, S, U...>))
                 || tester_for<Tag, Mode, T, Customization, T, S, U..., tutor<null_advisor>>
                 || tester_for<Tag, Mode, T, T, S, U..., tutor<null_advisor>>
      );
    }
  };

  /*! \brief generic function that generates a check from any class providing a static check method.

      This employs a \ref test_logger_primary "sentinel" and so can be used naively.
   */

  template<test_mode Mode, class Tag, class Customization, class T, class S, class... U>
    requires (is_equivalence_disambiguator_v<Tag> && implements_general_equivalence_check<Mode, Tag, Customization, T, S, U...>)
  bool general_equivalence_check(std::string_view description, test_logger<Mode>& logger, Tag, [[maybe_unused]] const value_based_customization<Customization>& customization, const T& value, const S& s, const U&... u)
  {
    using processor = equivalent_type_processor<S, U...>;

    const auto msg{
      [description, types{processor::info()}] (){
        return append_lines(description, "Comparison performed using:", make_type_info<value_tester<T>>(), "With equivalent types:", types)
          .append("\n");
      }()
    };

    sentinel<Mode> sentry{logger, msg};

    if constexpr(tester_for<Tag, Mode, T, Customization, T, S, U...>)
    {
      value_tester<T>::test(Tag{}, logger, customization.customizer, value, s, u...);
    }
    else if constexpr(tester_for<Tag, Mode, T, T, S, U...>)
    {
      value_tester<T>::test(Tag{}, logger, value, s, u...);
    }
    else if constexpr(processor::ends_with_tutor)
    {
      using delegator = equivalence_checker_delegator<Tag, T>;

      if constexpr(impl::invocable_without_last_arg<delegator, test_logger<Mode>&, Customization, T, S, U...>)
      {
        invoke_with_specified_args(delegator{}, std::make_index_sequence<sizeof...(U) + 3>(), logger, customization.customizer, value, s, u...);
      }
      else if constexpr(impl::invocable_without_last_arg<delegator, test_logger<Mode>&, T, S, U...>)
      {
        invoke_with_specified_args(delegator{}, std::make_index_sequence<sizeof...(U) + 2>(), logger, value, s, u...);
      }
      else
      {
        static_assert(dependent_false<value_tester<T>>::value, "Should never be triggered; indicates mismatch between requirement and logic");
      }
    }
    else
    {
      if constexpr(tester_for<Tag, Mode, T, Customization, T, S, U..., tutor<null_advisor>>)
      {
        value_tester<T>::test(Tag{}, logger, customization.customizer, value, s, u..., tutor<null_advisor>{});
      }
      else if constexpr(tester_for<Tag, Mode, T, T, S, U..., tutor<null_advisor>>)
      {
        value_tester<T>::test(Tag{}, logger, value, s, u..., tutor<null_advisor>{});
      }
      else
      {
        static_assert(dependent_false<value_tester<T>>::value, "Should never be triggered; indicates mismatch between requirement and logic");
      }
    }

    return !sentry.failure_detected();
  }

  template<bool IsFinalMessage, test_mode Mode, class Compare, class T, class Advisor>
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

  /*! \name dispatch_check basic overload set

      The next batch of functions form an overload set, dedicated to appropiately dispatching requests
      to check equality, binary relationships, equivalence and weak equivalence.

      In each case, the final argument may be utilized to supply custom advice, targeted at particular
      failures which may benefit from explanation. To active this, clients must supply a function
      object, with operator() overloaded to take two instances of a type and returning a string.
      Internally, reflection is used to invoke this function only when the matching type is obtained.
      Therefore, advice for type T can be supplied to a check of e.g. std::vector<T>, and the advice
      will find its way to the correct invocation site.

      \anchor dispatch_check_free_overloads

   */

  /*! \brief The workhorse of equality checking, which takes responsibility for reflecting upon types
      and then dispatching, appropriately.

      The input type, T, must either
      
       1. be equality comparable and/or possess a value_tester;
       2. be a range.

      If this constraint is not satisfied there are several possible remedies:

        1. Provide a specialization of value_tester or ensure operator== and !=
           exist, together with a specialization of serializer.

        2. If the invocation arises from a fallback from either check_equivalence or
           check_weak_equivalence then consider specializing either equivalence_checker or
           weak_equivalence_checker.
   */

  template<test_mode Mode, class Customization, class T, class Advisor=null_advisor>
    requires (    std::equality_comparable<T>
               || binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>>
               || sequoia::range<T>)
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, equality_check_t, const value_based_customization<Customization>&, const T& obtained, const T& prediction, [[maybe_unused]] tutor<Advisor> advisor={})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(std::equality_comparable<T>)
    {
      using finality = final_message_constant<!(binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>> || sequoia::range<T>)>;
      binary_comparison(finality{}, sentry, std::equal_to<T>{}, obtained, prediction, advisor);
    }

    if constexpr(binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(equality_check_t{}, logger, obtained, prediction, advisor);
    }
    else if constexpr (sequoia::range<T>)
    {
      check_range("", logger, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
    }

    return !sentry.failure_detected();
  }

  /*! \brief The workhorse for performing a check with respect to a user-specified binary operator 
  
      The comparison object either implements binary comparison for T or it is fed into check_range.
   */
  template<test_mode Mode, class Customization, class Compare, class T, class Advisor>
    requires (std::invocable<Compare, T, T> || sequoia::range<T>)
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, Compare compare, const value_based_customization<Customization>&, const T& obtained, const T& prediction, tutor<Advisor> advisor)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(std::invocable<Compare, T, T>)
    {
      using finality = final_message_constant<!sequoia::range<T>>;
      binary_comparison(finality{}, sentry, std::move(compare), obtained, prediction, advisor);
    }
    else
    {
      check_range("", logger, std::move(compare), value_based_customization<void>{}, obtained.begin(), obtained.end(), prediction.begin(), prediction.end(), advisor);
    }

    return !sentry.failure_detected();
  }

  /*! \brief The workhorse for (weak) equivalence checking

      This function will reflect on whether an appropriate specialization of (weak_) equivalence_checker exists.
      If so, it will be used and if not it will attempt to interpret T as a range. If both of these fail then:
      
      1. A weak equivalence check will attempt to fall back to an equivalence check;
      2. An equivalence check will attemp to fall back to a detailed equality check.

   */

  template<test_mode Mode, class Customization, class Tag, class T, class S, class... U>
    requires is_equivalence_disambiguator_v<Tag>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, Tag, const value_based_customization<Customization>& customization, const T& value, S&& s, U&&... u)
  {
    if constexpr (implements_general_equivalence_check<Mode, Tag, Customization, T, S, U...>)
    {
      return general_equivalence_check(description, logger, Tag{}, customization, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr (sequoia::range<T>)
    {
      return check_range(add_type_info<T>(description), logger, Tag{}, customization, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)), std::forward<U>(u)...);
    }
    else
    {
      using fallback = typename Tag::fallback;
      return dispatch_check(description, logger, fallback{}, customization, value, std::forward<S>(s), std::forward<U>(u)...);
    }
  }

 template<class Tag, test_mode Mode, class T, class Advisor>
   requires binary_tester_for<Tag, Mode, T, tutor<Advisor>>
 void select_test(Tag, test_logger<Mode>& logger, const T& obtained, const T& prediction, [[maybe_unused]] tutor<Advisor> advisor)
 {
   if constexpr(tester_for<Tag, Mode, T, T, T, tutor<Advisor>>)
   {
     value_tester<T>::test(Tag{}, logger, obtained, prediction, advisor);
   }
   else if constexpr(tester_for<Tag, Mode, T, T, T>)
   {
     value_tester<T>::test(Tag{}, logger, obtained, prediction);
   }
 }

 template<test_mode Mode, class T, class Advisor>
 inline constexpr bool does_detailed_agnostic_check{
      binary_tester_for<equality_check_t, Mode, T, tutor<Advisor>>
   || binary_tester_for<equivalence_check_t, Mode, T, tutor<Advisor>>
   || binary_tester_for<weak_equivalence_check_t, Mode, T, tutor<Advisor>>
   || binary_tester_for<agnostic_check_t, Mode, T, tutor<Advisor>>
 };

  /*! \brief The workhorse for generically dispatching checks to nested types.
  
   */

  template<test_mode Mode, class Customization, class T, class Advisor = null_advisor>
    requires (   std::equality_comparable<T> || does_detailed_agnostic_check<Mode, T, Advisor> || sequoia::range<T>)
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, agnostic_check_t, const value_based_customization<Customization>&, const T& obtained, const T& prediction, [[maybe_unused]] tutor<Advisor> advisor = {})
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(std::equality_comparable<T>)
    {
      using finality = final_message_constant<!(does_detailed_agnostic_check<Mode, T, Advisor> || sequoia::range<T>)>;
      binary_comparison(finality{}, sentry, std::equal_to<T>{}, obtained, prediction, advisor);
    }

    if constexpr(binary_tester_for<agnostic_check_t, Mode, T, tutor<Advisor>>)
    {
      select_test(agnostic_check_t{}, logger, obtained, prediction, advisor);
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
    else if constexpr(sequoia::range<T>)
    {
      check_range("", logger, agnostic_check_t{}, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
    }

    return !sentry.failure_detected();
  }

  template
  <
    test_mode Mode,
    class ElementDispatchDiscriminator,
    class Customization,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel,
    class Advisor=null_advisor
  >
  bool check_range(std::string_view description,
                   test_logger<Mode>& logger,
                   ElementDispatchDiscriminator discriminator,
                   const value_based_customization<Customization>& customization,
                   Iter first,
                   Sentinel last,
                   PredictionIter predictionFirst,
                   PredictionSentinel predictionLast,
                   tutor<Advisor> advisor={})
  {
    auto info{
      [description](){
        return description.empty() || description.back() == '\n'
          ? std::string{description} : std::string{description}.append("\n");
      }
    };

    sentinel<Mode> sentry{logger, info()};
    bool equal{true};

    using std::distance;
    using std::advance;

    const auto actualSize{fixed_width_unsigned_cast(distance(first, last))};
    const auto predictedSize{fixed_width_unsigned_cast(distance(predictionFirst, predictionLast))};
    if(check(equality, "Container size wrong", logger, actualSize, predictedSize))
    {
      auto predictionIter{predictionFirst};
      auto iter{first};
      for(; predictionIter != predictionLast; advance(predictionIter, 1), advance(iter, 1))
      {
        const auto dist{distance(predictionFirst, predictionIter)};
        auto mess{("Element " + std::to_string(dist)).append(" of range incorrect")};
        if(!dispatch_check(std::move(mess), logger, discriminator, customization, *iter, *predictionIter, advisor)) equal = false;
      }
    }
    else
    {
      equal = false;
    }

    return equal;
  }

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

  struct type_normalizer
  {
    template<class T>
    [[nodiscard]]
    const T& operator()(const T& val) const noexcept
    {
      return val;
    }

    template<class T>
      requires (!std::is_same_v<T, bool> && std::is_unsigned_v<T>)
    [[nodiscard]]
    auto operator()(T val) const noexcept
    {
      return fixed_width_unsigned_cast(val);
    }
  };

  //================= namespace-level convenience functions =================//

  template<test_mode Mode, class Compare, class T, class Advisor=null_advisor>
  bool check_relation(std::string_view description,
                      test_logger<Mode>& logger,
                      Compare&& compare,
                      const T& obtained,
                      const T& prediction,
                      tutor<Advisor> advisor={})
  {
    return dispatch_check(description, logger, std::forward<Compare>(compare), value_based_customization<void>{}, obtained, prediction, std::move(advisor));
  }

  template<test_mode Mode, class T, class Advisor=null_advisor>
  bool check(equality_check_t,
             std::string_view description,
             test_logger<Mode>& logger,
             const T& value,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    return dispatch_check(description, logger, equality_check_t{}, value_based_customization<void>{}, type_normalizer{}(value), type_normalizer{}(prediction), std::move(advisor));
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check(equivalence_check_t, std::string_view description, test_logger<Mode>& logger, const T& value, S&& s, U&&... u)
  {
    return dispatch_check(description, logger, equivalence_check_t{}, value_based_customization<void>{}, value, std::forward<S>(s), std::forward<U>(u)...);
  }

  template<class Customization, test_mode Mode, class T, class S, class... U>
  bool check(equivalence_check_t, std::string_view description, test_logger<Mode>& logger, const value_based_customization<Customization>& customization, const T& value, S&& s, U&&... u)
  {
    return dispatch_check(description, logger, equivalence_check_t{}, customization, value, std::forward<S>(s), std::forward<U>(u)...);
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check(weak_equivalence_check_t, std::string_view description, test_logger<Mode>& logger, const T& value, S&& s, U&&... u)
  {
    return dispatch_check(description, logger, weak_equivalence_check_t{}, value_based_customization<void>{}, value, std::forward<S>(s), std::forward<U>(u)...);
  }

  template<class Customization, test_mode Mode, class T, class S, class... U>
  bool check(weak_equivalence_check_t, std::string_view description, test_logger<Mode>& logger, const value_based_customization<Customization>& customization, const T& value, S&& s, U&&... u)
  {
    return dispatch_check(description, logger, weak_equivalence_check_t{}, customization, value, std::forward<S>(s), std::forward<U>(u)...);
  }

  template<test_mode Mode, class T, class Advisor=null_advisor>
  bool check(agnostic_check_t,
             std::string_view description,
             test_logger<Mode>& logger,
             const T& value,
             const T& prediction,
             tutor<Advisor> advisor={})
  {
    return dispatch_check(description, logger, agnostic_check_t{}, value_based_customization<void>{}, type_normalizer{}(value), type_normalizer{}(prediction), std::move(advisor));
  }

  template<test_mode Mode, class Advisor=null_advisor>
  bool check(std::string_view description, test_logger<Mode>& logger, const bool value, tutor<Advisor> advisor={})
  {
    return check(equality, description, logger, value, true, std::move(advisor));
  }

  template
  <
    test_mode Mode,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel,
    class Advisor = null_advisor
  >
  bool check_range(std::string_view description,
                   test_logger<Mode>& logger,
                   Iter first,
                   Sentinel last,
                   PredictionIter predictionFirst,
                   PredictionSentinel predictionLast,
                   tutor<Advisor> advisor={})
  {
    return check_range(description, logger, equality_check_t{}, value_based_customization<void>{}, first, last, predictionFirst, predictionLast, std::move(advisor));
  }

  template
  <
    test_mode Mode,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel,
    class Advisor = null_advisor
  >
  bool check_range_equivalence(std::string_view description,
                               test_logger<Mode>& logger,
                               Iter first,
                               Sentinel last,
                               PredictionIter predictionFirst,
                               PredictionSentinel predictionLast,
                               tutor<Advisor> advisor={})
  {
    return check_range(description, logger, equivalence_check_t{}, value_based_customization<void>{}, first, last, predictionFirst, predictionLast, std::move(advisor));
  }

  template
  <
    test_mode Mode,
    class Customization,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel,
    class Advisor = null_advisor
  >
  bool check_range_equivalence(std::string_view description,
                               test_logger<Mode>& logger,
                               const value_based_customization<Customization>& customization,
                               Iter first,
                               Sentinel last,
                               PredictionIter predictionFirst,
                               PredictionSentinel predictionLast,
                               tutor<Advisor> advisor={})
  {
    return check_range(description, logger, equivalence_check_t{}, customization, first, last, predictionFirst, predictionLast, std::move(advisor));
  }

  template
  <
    test_mode Mode,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel
  >
  bool check_range_weak_equivalence(std::string_view description,
                                    test_logger<Mode>& logger,
                                    Iter first,
                                    Sentinel last,
                                    PredictionIter predictionFirst,
                                    PredictionSentinel predictionLast)
  {
    return check_range(description, logger, weak_equivalence_check_t{}, value_based_customization<void>{}, first, last, predictionFirst, predictionLast);
  }

  template
  <
    test_mode Mode,
    class Customization,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel
  >
  bool check_range_weak_equivalence(std::string_view description,
                                    test_logger<Mode>& logger,
                                    const value_based_customization<Customization>& customization,
                                    Iter first,
                                    Sentinel last,
                                    PredictionIter predictionFirst,
                                    PredictionSentinel predictionLast)
  {
    return check_range(description, logger, weak_equivalence_check_t{}, customization, first, last, predictionFirst, predictionLast);
  }

  template
  <
    test_mode Mode,
    std::input_or_output_iterator Iter,
    std::sentinel_for<Iter> Sentinel,
    std::input_or_output_iterator PredictionIter,
    std::sentinel_for<PredictionIter> PredictionSentinel,
    class Advisor = null_advisor
  >
  bool check_range_agnostic(std::string_view description,
                   test_logger<Mode>& logger,
                   Iter first,
                   Sentinel last,
                   PredictionIter predictionFirst,
                   PredictionSentinel predictionLast,
                   tutor<Advisor> advisor={})
  {
    return check_range(description, logger, agnostic_check_t{}, value_based_customization<void>{}, first, last, predictionFirst, predictionLast, std::move(advisor));
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

    checker()
      : Extenders{logger()}...
    {}

    checker(const checker&)            = delete;
    checker& operator=(const checker&) = delete;

    template<class T, class Advisor=null_advisor>
    bool check(equality_check_t, std::string_view description, const T& value, const T& prediction, tutor<Advisor> advisor={})
    {
      return testing::check(equality, description, logger(), value, prediction, std::move(advisor));
    }

    template<class T, class Compare, class Advisor=null_advisor>
    bool check_relation(std::string_view description, Compare compare, const T& obtained, const T& prediction, tutor<Advisor> advisor={})
    {
      return testing::check_relation(description, logger(), std::move(compare), obtained, prediction, std::move(advisor));
    }

    template<class T, class S, class... U>
      requires (!is_value_customizer<T>)
    bool check(equivalence_check_t, std::string_view description, const T& value, S&& s, U&&... u)
    {
      return testing::check(equivalence, description, logger(), value_based_customization<void>{}, value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class Customization, class T, class S, class... U>
    bool check(equivalence_check_t, std::string_view description, const value_based_customization<Customization>& customization, const T& value, S&& s, U&&... u)
    {
      return testing::check(equivalence, description, logger(), customization, value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class T, class S, class... U>
      requires (!is_value_customizer<T>)
    bool check(weak_equivalence_check_t, std::string_view description, const T& value, S&& s, U&&... u)
    {
      return testing::check(weak_equivalence, description, logger(), value_based_customization<void>{}, value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class Customization, class T, class S, class... U>
    bool check(weak_equivalence_check_t, std::string_view description, const value_based_customization<Customization>&customization, const T& value, S&& s, U&&... u)
    {
      return testing::check(weak_equivalence, description, logger(), customization, value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class T, class Advisor = null_advisor>
    bool check(agnostic_check_t, std::string_view description, const T& value, const T& prediction, tutor<Advisor> advisor = {})
    {
      return testing::check(agnostic, description, logger(), value, prediction, std::move(advisor));
    }

    template<class Advisor=null_advisor>
    bool check(std::string_view description, const bool value, tutor<Advisor> advisor={})
    {
      return testing::check(description, logger(), value, std::move(advisor));
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
    bool check_range(std::string_view description,
                     Iter first,
                     Sentinel last,
                     PredictionIter predictionFirst,
                     PredictionSentinel predictionLast,
                     tutor<Advisor> advisor={})
    {
      return testing::check_range(description, logger(), first, last, predictionFirst, predictionLast, std::move(advisor));
    }

    template
    <
      std::input_or_output_iterator Iter,
      std::sentinel_for<Iter> Sentinel,
      std::input_or_output_iterator PredictionIter,
      std::sentinel_for<PredictionIter> PredictionSentinel,
      class Compare,
      class Advisor = null_advisor
    >
    bool check_range(std::string_view description,
                     Compare compare,
                     Iter first,
                     Sentinel last,
                     PredictionIter predictionFirst,
                     PredictionSentinel predictionLast,
                     tutor<Advisor> advisor={})
    {
      return testing::check_range(description, logger(), std::move(compare), value_based_customization<void>{}, first, last, predictionFirst, predictionLast, std::move(advisor));
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
      return logger().failure_messages();
    }

    void recovery(recovery_paths paths)
    {
      test_logger<Mode>::recovery(std::move(paths));
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
