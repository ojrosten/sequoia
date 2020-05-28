////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
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

#include "TestLogger.hpp"

namespace sequoia::testing
{
  /*! \brief class template, specializations of which implement detailed comparison of two instantiations of T; 
      \anchor detailed_equality_checker_primary
   */
  template<class T> struct detailed_equality_checker;

  /*! \brief class template, specializations of which implement comparision of two equivalent types
      \anchor equivalence_checker_primary
   */
  template<class T, class... Us> struct equivalence_checker;


  /*! \brief class template, specializations of which implement comparision of two weakly equivalent types;
      \anchor weak_equivalence_checker_primary
   */
  template<class T, class... Us> struct weak_equivalence_checker;

  struct equality_tag{};
  struct equivalence_tag{};
  struct weak_equivalence_tag{};
  
  template<class T> constexpr bool has_equivalence_checker_v{class_template_is_instantiable_v<equivalence_checker, T>};
  template<class T> constexpr bool has_weak_equivalence_checker_v{class_template_is_instantiable_v<weak_equivalence_checker, T>};
  template<class T> constexpr bool has_detailed_equality_checker_v{class_template_is_instantiable_v<detailed_equality_checker, T>};

  /*! \brief Specialize this struct template to provide custom serialization of a given class.
      \anchor serializer_primary
   */
  
  template<class T>
  struct serializer
  {
    static_assert(is_serializable_v<T>);
    
    [[nodiscard]]
    static std::string make(const T& val)
    {        
      std::ostringstream os{};
      os << std::boolalpha << val;
      return os.str();
    }
  };

  template<class T>
  [[nodiscard]]
  std::string to_string(const T& value)
  {
    return serializer<T>::make(value);    
  }
    
  template<class T>
  [[nodiscard]]
  std::string demangle()
  {
    #ifndef _MSC_VER_
      int status;
      return abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
    #else        
      return typeid(T).name();
    #endif
  }

  /*! \brief Specialize this struct template to customize the way in which type info is generated for a given class.
      This is particularly useful for class templates where standard de-mangling may be hard to read!
      
      \anchor type_demangler_primary
   */

  template<class T, class... U>
  struct type_demangler
  {
    [[nodiscard]]
    static std::string make()
    {
      auto info{std::string{'['}.append(demangle<T>())};
      if constexpr(sizeof...(U) > 0)
        info.append("\n;").append(type_demangler<U...>::make());

      info += ']';
      return info;
    }
  };

  template<class T, class... U>
  [[nodiscard]]
  std::string add_type_info(std::string_view description)
  {
    return merge(description, type_demangler<T, U...>::make().append("\n"), description.empty() ? "" : "\n");
  }

  /*! \brief generic function that generates a check from any class providing a static check method.

      This employs a \ref test_logger_primary "sentinel" and so can be used naively.
   */
  
  template<class EquivChecker, test_mode Mode, class T, class S, class... U>
  bool general_equivalence_check(std::string_view description, test_logger<Mode>& logger, const T& value, const S& s, const U&... u)
  {
    const std::string message{
      add_type_info<S, U...>(
        merge(description, "Comparison performed using:\n\t" + type_demangler<EquivChecker>::make() + "\n\tWith equivalent types:", "\n"))
    };
      
    sentinel<Mode> r{logger, message};
    const auto previousFailures{logger.failures()};
    
    EquivChecker::check(message, logger, value, s, u...);
      
    return logger.failures() == previousFailures;
  }

  [[nodiscard]]
  std::string operator_message(std::string_view description, std::string_view typeInfo, std::string_view op, std::string_view retVal);

  [[nodiscard]]
  std::string prediction_message(std::string_view obtained, std::string_view predicted);

  /*! \name dispatch_check basic overload set

      The next three functions form an overload set, dedicated to appropiately dispatching requests
      to check equality, equivalence and weak equivalence. This set may be supplemented by extenders
      of the testing framework, see FuzzyTestCore.hpp for an example.

      \anchor dispatch_check_free_overloads

   */

  /*! \brief The workhorse of equality checking, which takes responsibility for reflecting upon types
      and then dispatching, appropriately. 

      The input type, T, must either be equality comparable or possess a detailed_equality_checker, or both. 
      Generally, it will be the case that T does indeed overload operator==; anything beyond 
      the simplest user-defined types should be furnished with a detailed_equality_checker.
   */
  
  template<test_mode Mode, class T>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, equality_tag, const T& value, const T& prediction)
  {
    constexpr bool delegate{has_detailed_equality_checker_v<T> || is_container_v<T>};

    static_assert(delegate || is_equal_to_comparable_v<T>,
                  "Provide either a specialization of detailed_equality_checker or ensure operator== exists,"
                  "together with a specialization of serializer");

    sentinel<Mode> s{logger, add_type_info<T>(description)};
     
    if constexpr(is_equal_to_comparable_v<T>)
    {
      s.log_check();
      if(!(prediction == value))
      {
        auto message{operator_message(description, add_type_info<T>(""), "==", "false")};
        if constexpr(!delegate)
        {
          message.append(prediction_message(to_string(value), to_string(prediction)));      
        }
        logger.log_failure(message);
      }
    }

    if constexpr(delegate)
    {
      std::string_view desc{s.checks_registered() ? "" : description};
      if constexpr(has_detailed_equality_checker_v<T>)
      {
        detailed_equality_checker<T>::check(desc, logger, value, prediction);
      }
      else if constexpr(is_container_v<T>)
      {
        check_range(desc, logger, std::begin(value), std::end(value), std::begin(prediction), std::end(prediction));
      }      
      else
      {
        static_assert(dependent_false<T>::value, "Unable to check detailed equality for Type");
      }
    }

    return !s.failure_detected();
  }

  /*! \brief The workhorse for equivalence checking

      This function will reflect on whether an appropriate specialization of equivalence_checker exists.
      If so, it will be used and if not it will attempt to interpret T as a range. Only if this fails
      then a static assertion will terminate compilation.

   */
  
  template<test_mode Mode, class T, class S, class... U>
  bool dispatch_check(std::string_view description, test_logger<Mode>& logger, equivalence_tag, const T& value, S&& s, U&&... u)
  {
    if constexpr(class_template_is_instantiable_v<equivalence_checker, T>)
    {      
      return general_equivalence_check<equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(is_container_v<T> && (sizeof...(U) == 0))
    {
      return check_range(add_type_info<T>(description), logger, equivalence_tag{}, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)));
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
    if constexpr(class_template_is_instantiable_v<weak_equivalence_checker, T>)
    {      
      return general_equivalence_check<weak_equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(is_container_v<T> && (sizeof...(U) == 0))
    {
      return check_range(add_type_info<T>(description), logger, weak_equivalence_tag{}, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)));
    }
    else
    {
      static_assert(dependent_false<T>::value, "Unable to find an appropriate specialization of the weak_equivalence_checker");
    }
  }

  template<test_mode Mode, class ElementDispatchDiscriminator, class Iter, class PredictionIter>
  bool check_range(std::string_view description, test_logger<Mode>& logger, ElementDispatchDiscriminator discriminator, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    sentinel<Mode> r{logger, description};
    bool equal{true};

    using std::distance;
    using std::advance;

    const auto predictedSize{distance(predictionFirst, predictionLast)};
    if(check_equality(merge(description, "Container size wrong", "\n"), logger, distance(first, last), predictedSize))
    {
      auto predictionIter{predictionFirst};
      auto iter{first};
      for(; predictionIter != predictionLast; advance(predictionIter, 1), advance(iter, 1))
      {
        const auto dist{distance(predictionFirst, predictionIter)};
        std::string_view desc{dist ? "" : description};
        std::string mess{merge(desc, "Element ", "\n")
            .append(std::to_string(dist)).append(" of range incorrect")};
        if(!dispatch_check(std::move(mess), logger, discriminator, *iter, *predictionIter)) equal = false;
      }
    }
    else
    {
      equal = false;
    }
      
    return equal;
  }

  template<class E, test_mode Mode, class Fn>
  bool check_exception_thrown(std::string_view description, test_logger<Mode>& logger, Fn&& function)
  {
    const std::string message{"\t" + add_type_info<E>(merge(description, "Expected Exception Type:", "\n"))};
    sentinel<Mode> r{logger, message};
    r.log_check();
    try
    {
      function();
      logger.log_failure(merge(message, "No exception thrown\n"));
      return false;
    }
    catch(const E&)
    {
      return true;
    }
    catch(const std::exception& e)
    {
      logger.log_failure(merge(message, std::string{"Unexpected exception thrown (caught by std::exception&):\n\t\""} + e.what() + "\"\n"));
      return false;
    }
    catch(...)
    {
      logger.log_failure(merge(message, "Unknown exception thrown\n"));
      return false;
    }
  }

  //================= namespace-level convenience functions =================//

  template<test_mode Mode, class T>
  bool check_equality(std::string_view description, test_logger<Mode>& logger, const T& value, const T& prediction)
  {
    return dispatch_check(description, logger, equality_tag{}, value, prediction);
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

  template<test_mode Mode>
  bool check(std::string_view description, test_logger<Mode>& logger, const bool value)
  {
    return check_equality(description, logger, value, true);
  }
    
  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range(std::string_view description, test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return check_range(description, logger, equality_tag{}, first, last, predictionFirst, predictionLast);      
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

  template<test_mode Mode, class... Extenders>
  class checker : private test_logger<Mode>, public Extenders...
  {
  public:
    constexpr static test_mode mode{Mode};
    using logger_type = test_logger<Mode>;

    static_assert(((sizeof(Extenders) == sizeof(logger_type*)) && ...),
                  "The state of any Extenders must comprise precisely a reference to a Logger");
 
    static_assert(((mode == Extenders::mode) && ...));
      
    checker() : Extenders{logger()}... {}
      
    checker(const checker&)            = delete;
    checker& operator=(const checker&) = delete;      
    checker& operator=(checker&&)      = delete;

    template<class T>
    bool check_equality(std::string_view description, const T& value, const T& prediction)
    {
      return testing::check_equality(description, logger(), value, prediction);
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

    bool check(std::string_view description, const bool value)
    {
      return testing::check(description, logger(), value);
    }

    template<class E, class Fn>
    bool check_exception_thrown(std::string_view description, Fn&& function)
    {
      return testing::check_exception_thrown<E>(description, logger(), std::forward<Fn>(function));
    }

    template<class Iter, class PredictionIter>
    bool check_range(std::string_view description, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return testing::check_range(description, logger(), first, last, predictionFirst, predictionLast);
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

    ~checker() = default;

    void log_critical_failure(std::string_view message) { logger().log_critical_failure(message); }
      
    void log_failure(std::string_view message) { logger().log_failure(message); }

    [[nodiscard]]
    std::size_t checks() const noexcept { return logger().checks(); }

    [[nodiscard]]
    std::size_t failures() const noexcept { return logger().failures(); }

    [[nodiscard]]
    std::string_view current_message() const noexcept{ return logger().current_message(); }

    [[nodiscard]]
    int exceptions_detected_by_sentinel() const noexcept { return logger().exceptions_detected_by_sentinel(); }

    [[nodiscard]]
    sentinel<Mode> make_sentinel(std::string_view message)
    {
      return {logger(), message};
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
