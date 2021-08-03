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
    \ref detailed_equality_checker_primary "detailed_equality_checker" is defined. Specializations
    perform a detailed check of the equality
    of the state of two supposedly equal instance of a class. Note that there is no need for the
    detailed_equality_checker to include checks of operator== or operator!=, since these will be done,
    regardless. Indeed, the detailed_equality_checker will be triggered independently of whether operator==
    fails since either the latter or the accessors may have bugs, and introducing unnecessary dependencies
    would reduce the fidelity of the testing framework.

    If a detailed_equality_checker is supplied, then compile-time logic will ignore any attempt to
    serialize objects of type T. Typically, clients may expect a notification that operator== has returned
    false (and, for consistency, notification that operator!= has returned true). There will then be a series of
    subsequent checks which will reveal the precise nature of the failure. For example, for vectors, one
    will be told whether the sizes differ and, if not, the element which is causing the difference
    between the two supposedly equal instances. If the vector holds a user-defined type then, so long as
    this has its own detailed_equality_checker, the process will continue until a type is reached without
    a detailed_equality_checker; typically this will be a sufficiently simple type that serialization is
    the appropriate solution (as is the case for may built-in types).

    Suppose a client wishes to compare instance of some_container<T>. If some_container has a specialization
    of \ref detailed_equality_checker_primary "detailed_equality_checker" then this will be used; if it does
    not then reflection is used to
    determine if some_container overloads begin and end. If so, then it is treated as a range and
    all that is required is to implement a detailed_equality_checker for T (unless serialization is prefered).
    If some_container is user-defined, it is wisest to provide an overload of the
    \ref detailed_equality_checker_primary "detailed_equality_checker".
    However, if the container is part of std, it is probably safe to assume it works correctly and so
    instead effort can be directed focused on T.

    With this in mind, imagine creating a container. One of the first things one may wish to do is to check
    that it is correctly initialized. It would be a mistake to use the
    \ref detailed_equality_checker_primary "detailed_equality_checker" for this
    since, to do so, a second, identical instance would need to be created. This is circular and prone to
    false positives. Consequently, the testing framework also defines a pair of class templates that
    complement
    \ref detailed_equality_checker_primary "detailed_equality_checker":
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
#include "sequoia/TestFramework/Advice.hpp"
#include "sequoia/TestFramework/TestLogger.hpp"
#include "sequoia/Core/Meta/Utilities.hpp"

#include <functional>

namespace sequoia::testing
{
  template<class Checker, test_mode Mode, class... Args>
  concept checker_for = requires(test_logger<Mode>& logger, Args&&... args) {
    Checker::check(logger, std::forward<Args>(args)...);
  };

  template<class... Ts>
  struct equivalent_type_processor
  {
    static constexpr bool ends_with_tutor{};

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
    requires derived_from<E, std::exception>
  struct exception_message_extractor<E>
  {
    static std::string get(const E& e) { return e.what(); }
  };

  template<class E>
    requires (!derived_from<E, std::exception>) && serializable<E>
  struct exception_message_extractor<E>
  {
    static std::string get(const E& e) { return to_string(e); }
  };
  
  template<class Compare>
  struct failure_reporter
  {
    template<class T>
    [[nodiscard]]
    static std::string report(const Compare&, const T&, const T&) = delete;
  };

  /*! \brief generic function that generates a check from any class providing a static check method.

      This employs a \ref test_logger_primary "sentinel" and so can be used naively.
   */

  template<class EquivChecker, test_mode Mode, class T, class S, class... U>
  bool general_equivalence_check(std::string_view description, test_logger<Mode>& logger, const T& value, const S& s, const U&... u)
  {
    constexpr equivalent_type_processor<S, U...> processor{};

    const auto msg{
      [description, types{processor.info()}](){
        return append_lines(description, "Comparison performed using:", make_type_info<EquivChecker>(), "With equivalent types:", types)
          .append("\n");
      }()
    };

    sentinel<Mode> sentry{logger, msg};

    if constexpr(checker_for<EquivChecker, Mode, T, S, U...>)
    {
      EquivChecker::check(logger, value, s, u...);
    }
    else if constexpr(processor.ends_with_tutor)
    {
      auto fn{
        [&logger,&value](auto&&... predictions){
          EquivChecker::check(logger, value, std::forward<decltype(predictions)>(predictions)...);
        }
      };

      invoke_with_specified_args(fn, std::make_index_sequence<sizeof...(u)>(), s, u...);
    }
    else
    {
      EquivChecker::check(logger, value, s, u..., tutor<null_advisor>{});
    }

    return !sentry.failure_detected();
  }

  /*! \name dispatch_check basic overload set

      The next three functions form an overload set, dedicated to appropiately dispatching requests
      to check equality, equivalence and weak equivalence. This set may be supplemented by extenders
      of the testing framework, see RelationalTestCore.hpp for an example.

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

      The input type, T, must either be equality comparable or possess a detailed_equality_checker, or both.
      Generally, it will be the case that T does indeed overload operator==; anything beyond
      the simplest user-defined types should be furnished with a detailed_equality_checker.
   */

  template<test_mode Mode, class T, class Advisor=null_advisor>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, equality_tag, const T& obtained, const T& prediction, [[maybe_unused]] tutor<Advisor> advisor=tutor<Advisor>{})
  {
    constexpr bool delegate{has_detailed_equality_checker_v<T> || range<T>};

    static_assert(delegate || equality_comparable<T>,
                  "Provide either a specialization of detailed_equality_checker or"
                  "ensure operator== and != exists, together with a specialization of serializer");

    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(equality_comparable<T>)
    {
      sentry.log_check();
      if(!(prediction == obtained))
      {
        auto message{failure_message(obtained, prediction)};

        append_advice(message, {advisor, obtained, prediction});

        sentry.log_failure(std::move(message));
      }
    }

    if constexpr(delegate)
    {
      if constexpr(has_detailed_equality_checker_v<T>)
      {
        if constexpr(checker_for<detailed_equality_checker<T>, Mode, T, T, tutor<Advisor>>)
        {
          detailed_equality_checker<T>::check(logger, obtained, prediction, advisor);
        }
        else
        {
          detailed_equality_checker<T>::check(logger, obtained, prediction);
        }
      }
      else if constexpr(range<T>)
      {
        check_range("", logger, std::begin(obtained), std::end(obtained), std::begin(prediction), std::end(prediction), advisor);
      }
      else
      {
        static_assert(dependent_false<T>::value, "Unable to check detailed equality for Type");
      }
    }

    return !sentry.failure_detected();
  }

  
  /*! \brief Adds to the overload set dispatch_check_free_overloads

   */
  template<test_mode Mode, class Compare, class T, class Advisor>
    requires (!same_as<Compare, weak_equivalence_tag> && !same_as<Compare, weak_equivalence_tag>)
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, Compare c, const T& obtained, const T& prediction, tutor<Advisor> advisor)
  {
    sentinel<Mode> sentry{logger, add_type_info<T>(description)};

    if constexpr(invocable<Compare, T, T>)
    {
      sentry.log_check();
      if(!c(obtained, prediction))
      {
        std::string message{failure_reporter<Compare>::report(c, obtained, prediction)};
        append_advice(message, {advisor, obtained, prediction});

        sentry.log_failure(message);
      }

      return !sentry.failure_detected();
    }
    else if constexpr(range<T>)
    {
      return check_range("", logger, std::move(c), obtained.begin(), obtained.end(), prediction.begin(), prediction.end(), advisor);
    }
    else
    {
      static_assert(dependent_false<T>::value, "Compare cannot consume T directly nor interpret as a range");
    }
  }

  /*! \brief The workhorse for equivalence checking

      This function will reflect on whether an appropriate specialization of equivalence_checker exists.
      If so, it will be used and if not it will attempt to interpret T as a range. Only if this fails
      then a static assertion will terminate compilation.

   */

  template<test_mode Mode, class T, class S, class... U>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, equivalence_tag, const T& value, S&& s, U&&... u)
  {
    if constexpr(class_template_is_default_instantiable<equivalence_checker, T>)
    {
      return general_equivalence_check<equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(range<T>)
    {
      return check_range(add_type_info<T>(description), logger, equivalence_tag{}, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)), std::forward<U>(u)...);
    }
    else
    {
      static_assert(dependent_false<T>::value, "Unable to find an appropriate specialization of the equivalence_checker");
    }
  }

  /*! \brief The workhorse for weak equivalence checking

      This function will reflect on whether an appropriate specialization of weak_equivalence_checker exists.
      If so, it will be used and if not it will attempt to interpret T as a range. Only if this fails then a
      static assertion will terminate compilation.

   */

  template<test_mode Mode, class T, class S, class... U>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u)
  {
    if constexpr(class_template_is_default_instantiable<weak_equivalence_checker, T>)
    {
      return general_equivalence_check<weak_equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(range<T>)
    {
      return check_range(add_type_info<T>(description), logger, weak_equivalence_tag{}, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)), std::forward<U...>(u)...);
    }
    else
    {
      static_assert(dependent_false<T>::value, "Unable to find an appropriate specialization of the weak_equivalence_checker");
    }
  }

  template<test_mode Mode, class ElementDispatchDiscriminator, class Iter, class PredictionIter, class Advisor=null_advisor>
  bool check_range(std::string_view description, test_logger<Mode>& logger, ElementDispatchDiscriminator discriminator, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor=tutor<Advisor>{})
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
    if(check_equality("Container size wrong", logger, actualSize, predictedSize))
    {
      auto predictionIter{predictionFirst};
      auto iter{first};
      for(; predictionIter != predictionLast; advance(predictionIter, 1), advance(iter, 1))
      {
        const auto dist{distance(predictionFirst, predictionIter)};
        auto mess{std::string{"Element "}
            .append(std::to_string(dist)).append(" of range incorrect")};
        if(!dispatch_check(std::move(mess), logger, discriminator, *iter, *predictionIter, advisor)) equal = false;
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

  //================= namespace-level convenience functions =================//

  template<test_mode Mode, class Compare, class T, class Advisor=null_advisor>
  bool check_relation(std::string_view description,
                      test_logger<Mode>& logger,
                      Compare&& compare,
                      const T& obtained,
                      const T& prediction,
                      tutor<Advisor> advisor=tutor<Advisor>{})
  {
    return dispatch_check(description, logger, std::forward<Compare>(compare), obtained, prediction, std::move(advisor));
  }
  
  template<test_mode Mode, class T, class Advisor=null_advisor>
  bool check_equality(std::string_view description,
                      test_logger<Mode>& logger,
                      const T& value,
                      const T& prediction,
                      tutor<Advisor> advisor=tutor<Advisor>{})
  {
    auto transformer{
      [](const T& val) -> decltype(auto) {
        if constexpr(!std::is_same_v<T, bool> && std::is_unsigned_v<T>)
           return fixed_width_unsigned_cast(val);
        else
          return val;
      }
    };

    return dispatch_check(description, logger, equality_tag{}, transformer(value), transformer(prediction), std::move(advisor));
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check_equivalence(std::string_view description, test_logger<Mode>& logger, const T& value, S&& s, U&&... u)
  {
    return dispatch_check(description, logger, equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)...);
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check_weak_equivalence(std::string_view description, test_logger<Mode>& logger, const T& value, S&& s, U&&... u)
  {
    return dispatch_check(description, logger, weak_equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)...);
  }

  template<test_mode Mode, class Advisor=null_advisor>
  bool check(std::string_view description, test_logger<Mode>& logger, const bool value, tutor<Advisor> advisor=tutor<Advisor>{})
  {
    return check_equality(description, logger, value, true, std::move(advisor));
  }

  template<test_mode Mode, class Iter, class PredictionIter, class Advisor=null_advisor>
  bool check_range(std::string_view description, test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor=tutor<Advisor>{})
  {
    return check_range(description, logger, equality_tag{}, first, last, predictionFirst, predictionLast, std::move(advisor));
  }

  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range_equivalence(std::string_view description, test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return check_range(description, logger, equivalence_tag{}, first, last, predictionFirst, predictionLast);
  }

  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range_weak_equivalence(std::string_view description, test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return check_range(description, logger, weak_equivalence_tag{}, first, last, predictionFirst, predictionLast);
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
  concept extender = requires(test_logger<Mode>& logger) {
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
    bool check_equality(std::string_view description, const T& value, const T& prediction, tutor<Advisor> advisor=tutor<Advisor>{})
    {
      return testing::check_equality(description, logger(), value, prediction, std::move(advisor));
    }

    template<class T, class Compare, class Advisor=null_advisor>
    bool check_relation(std::string_view description, Compare compare, const T& obtained, const T& prediction, tutor<Advisor> advisor=tutor<Advisor>{})
    {
      return testing::check_relation(description, logger(), std::move(compare), obtained, prediction, std::move(advisor));
    }

    template<class T, class S, class... U>
    bool check_equivalence(std::string_view description, const T& value, S&& s, U&&... u)
    {
      return testing::check_equivalence(description, logger(), value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class T, class S, class... U>
    bool check_weak_equivalence(std::string_view description, const T& value, S&& s, U&&... u)
    {
      return testing::check_weak_equivalence(description, logger(), value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class Advisor=null_advisor>
    bool check(std::string_view description, const bool value, tutor<Advisor> advisor=tutor<Advisor>{})
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

    template<class Iter, class PredictionIter, class Advisor=null_advisor>
    bool check_range(std::string_view description, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor=tutor<Advisor>{})
    {
      return testing::check_range(description, logger(), first, last, predictionFirst, predictionLast, std::move(advisor));
    }

    template<class Iter, class PredictionIter, class Compare, class Advisor=null_advisor>
    bool check_range(std::string_view description, Compare compare, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, tutor<Advisor> advisor=tutor<Advisor>{})
    {
      return testing::check_range(description, logger(), std::move(compare), first, last, predictionFirst, predictionLast, std::move(advisor));
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

    checker& operator=(checker&&) noexcept = default;

    ~checker() = default;

    [[nodiscard]]
    std::size_t checks() const noexcept { return logger().checks(); }

    [[nodiscard]]
    std::size_t failures() const noexcept { return logger().failures(); }

    [[nodiscard]]
    int exceptions_detected_by_sentinel() const noexcept { return logger().exceptions_detected_by_sentinel(); }

    [[nodiscard]]
    sentinel<Mode> make_sentinel(std::string_view message)
    {
      return {logger(), message};
    }

    [[nodiscard]]
    const std::string& top_level_message() const
    {
      return logger().top_level_message();
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
