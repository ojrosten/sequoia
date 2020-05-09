////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file FreeCheckers.hpp
    \brief Free functions for performing checks, together with the 'checker' class template which wraps them.

    Given a type, T, any reasonable testing framework must provide a mechanism for checking whether or
    not two instances of T are, in the some sense, the same.
*/

#include "FreeCheckersDetails.hpp"

namespace sequoia::unit_testing
{  
  template <class, template<class...> class T, class... Args>
  struct template_class_is_instantiable : std::false_type
  {};

  template <template<class...> class T, class... Args>
  struct template_class_is_instantiable<std::void_t<decltype(T<Args...>{})>, T, Args...>
    : std::true_type
  {};

  template <template<class...> class T, class... Args>
  constexpr bool template_class_is_instantiable_v{template_class_is_instantiable<std::void_t<>, T, Args...>::value};

  template<class T> struct detailed_equality_checker;
  template<class T, class... Us> struct equivalence_checker;
  template<class T, class... Us> struct weak_equivalence_checker;

  struct equality_tag{};
  struct equivalence_tag{};
  struct weak_equivalence_tag{};
  
  template<class T> constexpr bool has_equivalence_checker_v{template_class_is_instantiable_v<equivalence_checker, T>};
  template<class T> constexpr bool has_weak_equivalence_checker_v{template_class_is_instantiable_v<weak_equivalence_checker, T>};
  template<class T> constexpr bool has_detailed_equality_checker_v{template_class_is_instantiable_v<detailed_equality_checker, T>};
  
  template<class T, class=std::void_t<>>
  struct is_serializable : public std::false_type
  {};

  // This makelval hack is to work around a bug in the XCode 10.2 stl implementation:
  // ostream line 1036. Without this, e.g. decltype(std::stringstream{} << std::vector<int>{})
  // deduces stringstream&
  template<class T>
  std::add_lvalue_reference_t<T> makelval() noexcept;
    
  template<class T>
  struct is_serializable<T, std::void_t<decltype(makelval<std::stringstream>() << std::declval<T>())>>
    : public std::true_type
  {};

  template<class T> constexpr bool is_serializable_v{is_serializable<T>::value};

  template<class T>
  struct string_maker
  {
    static_assert(is_serializable_v<T>, "Either provide a specialization of string_maker or a specialization of detailed_equality_checker; the latter will enter a different if constexpr block, avoiding the need for serializing T directly");
    
    [[nodiscard]]
    static std::string make(const T& val)
    {        
      std::ostringstream os{};
      os << std::boolalpha << val;
      return os.str();
    }
  };

  template<class T>
  [[nodiscard]] std::string to_string(const T& value)
  {
    return string_maker<T>::make(value);    
  }

  template<class T, class... U>
  [[nodiscard]]
  std::string make_type_info()
  {        
    std::string info{demangle<T>()};
    if constexpr(sizeof...(U) > 0)
      info.append("\n;").append(make_type_info<U...>());

    return info;
  }

  template<class T, class... U>
  [[nodiscard]]
  std::string add_type_info(std::string_view description)
  {
    return combine_messages(description, "[" + make_type_info<T, U...>() + "]\n", description.empty() ? "" : "\n");
  }

  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range(std::string_view description, unit_test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast);

  template<test_mode Mode, class T>
  bool check(std::string_view description, unit_test_logger<Mode>& logger, equality_tag, const T& value, const T& prediction)
  {
    constexpr bool delegate{has_detailed_equality_checker_v<T> || is_container_v<T>};

    static_assert(delegate || is_equal_to_comparable_v<T>,
                  "Provide either a specialization of detailed_equality_checker or ensure operator== exists, together with a specialization of string_maker");
      
    using sentinel = typename unit_test_logger<Mode>::sentinel;      
    sentinel s{logger, add_type_info<T>(description)};

    const auto priorFailures{logger.failures()};
      
    auto messageGenerator{
      [description](std::string op, std::string retVal){
        std::string info{add_type_info<T>("")
            .append("\toperator").append(std::move(op))
            .append(" returned ").append(std::move(retVal)).append("\n")
            };
        return description.empty() ? std::move(info) : std::string{"\t"}.append(description).append("\n" + std::move(info));
      }
    };
      
    if constexpr(is_equal_to_comparable_v<T>)
    {
      s.log_check();
      if(!(prediction == value))
        {
          auto message{messageGenerator("==", "false")};
          if constexpr(!delegate)
          {
            message.append("\tObtained : " + to_string(value) + "\n");
            message.append("\tPredicted: " + to_string(prediction) + "\n\n");           
          }
          logger.log_failure(message);
        }
    }

    if constexpr(delegate)
    {
      if constexpr(is_not_equal_to_comparable_v<T>)
      {
        s.log_check();
        if(prediction != value)
        {
          logger.log_failure(messageGenerator("!=", "true"));
        }
      }

      if constexpr(has_detailed_equality_checker_v<T>)
      {
        detailed_equality_checker<T>::check(description, logger, value, prediction);
      }
      else if constexpr(is_container_v<T>)
      {
        check_range(description, logger, std::begin(value), std::end(value), std::begin(prediction), std::end(prediction));
      }      
      else
      {
        static_assert(dependent_false<T>::value, "Unable to check detailed equality for Type");
      }
    }

    return logger.failures() == priorFailures;
  }
  
  template<test_mode Mode, class T, class S, class... U>
  bool check(std::string_view description, unit_test_logger<Mode>& logger, equivalence_tag, const T& value, S&& s, U&&... u)
  {
    if constexpr(template_class_is_instantiable_v<equivalence_checker, T>)
    {      
      return impl::check<equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(is_container_v<T> && (sizeof...(U) == 0))
    {
      return check_range_equivalence(add_type_info<T>(description), logger, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)));
    }
    else
    {
      static_assert(dependent_false<T>::value, "Unable to find an appropriate specialization of the equivalence_checker");
    }
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check(std::string_view description, unit_test_logger<Mode>& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u)
  {
    if constexpr(template_class_is_instantiable_v<weak_equivalence_checker, T>)
    {      
      return impl::check<weak_equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
    }
    else if constexpr(is_container_v<T> && (sizeof...(U) == 0))
    {
      return check_range_weak_equivalence(add_type_info<T>(description), logger, std::begin(value), std::end(value), std::begin(std::forward<S>(s)), std::end(std::forward<S>(s)));
    }
    else
    {
      static_assert(dependent_false<T>::value, "Unable to find an appropriate specialization of the weak_equivalence_checker");
    }
  }
    
  template<test_mode Mode, class T> bool check_equality(std::string_view description, unit_test_logger<Mode>& logger, const T& value, const T& prediction)
  {
    return check(description, logger, equality_tag{}, value, prediction);
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check_equivalence(std::string_view description, unit_test_logger<Mode>& logger, const T& value, S&& s, U&&... u)
  {
    return check(description, logger, equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)...);      
  }

  template<test_mode Mode, class T, class S, class... U>
  bool check_weak_equivalence(std::string_view description, unit_test_logger<Mode>& logger, const T& value, S&& s, U&&... u)
  {
    return check(description, logger, weak_equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)...);      
  }

  template<test_mode Mode> bool check(std::string_view description, unit_test_logger<Mode>& logger, const bool value)
  {
    return check_equality(description, logger, value, true);
  }

  template<class E, test_mode Mode, class Fn>
  bool check_exception_thrown(std::string_view description, unit_test_logger<Mode>& logger, Fn&& function)
  {
    const std::string message{"\t" + add_type_info<E>(combine_messages(description, "Expected Exception Type:", "\n"))};
    typename unit_test_logger<Mode>::sentinel r{logger, message};
    r.log_check();
    try
    {
      function();
      logger.log_failure(combine_messages(message, "No exception thrown\n"));
      return false;
    }
    catch(const E&)
    {
      return true;
    }
    catch(const std::exception& e)
    {
      logger.log_failure(combine_messages(message, std::string{"Unexpected exception thrown (caught by std::exception&):\n\t\""} + e.what() + "\"\n"));
      return false;
    }
    catch(...)
    {
      logger.log_failure(combine_messages(message, "Unknown exception thrown\n"));
      return false;
    }
  }
    
  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range(std::string_view description, unit_test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return impl::check_range(description, logger, equality_tag{}, first, last, predictionFirst, predictionLast);      
  }

  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range_equivalence(std::string_view description, unit_test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return impl::check_range(description, logger, equivalence_tag{}, first, last, predictionFirst, predictionLast);      
  }

  template<test_mode Mode, class Iter, class PredictionIter>
  bool check_range_weak_equivalence(std::string_view description, unit_test_logger<Mode>& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
  {
    return impl::check_range(description, logger, weak_equivalence_tag{}, first, last, predictionFirst, predictionLast);      
  }

  /*! \class checker
      \brief Exposes elementary check methods, with the option to plug in arbitrary Extenders to compose functionality.

      This class template is templated on the enum class test_mode, together with a variadic set of Extenders.

      In its unextended form, the class is appropriate for plugging into basic_test to generate a base class
      appropriate for testing free functions. Within the unit test framework various Extenders are defined.
      For example, there are extensions to test types with regular semantics, types with move-only semantics, to do 
      performance tests, and more, besides. The template design allows extenders to be conveniently mixed and
      matched via using declarations.

      Each extender must be initialized with a reference to the unit_test_logger held by the checker.
      To ensure the correct order of initialization, the unit_test_logger is inherited privately.
   */

  template<test_mode Mode, class... Extenders>
  class checker : private unit_test_logger<Mode>, public Extenders...
  {
  public:    
    constexpr static test_mode mode{Mode};
    using logger_type = unit_test_logger<Mode>;

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
      return unit_testing::check_equality(description, logger(), value, prediction);
    }

    template<class T, class S, class... U>
    bool check_equivalence(std::string_view description, const T& value, S&& s, U&&... u)
    {
      return unit_testing::check_equivalence(description, logger(), value, std::forward<S>(s), std::forward<U>(u)...);
    }

    template<class T, class S, class... U>
    bool check_weak_equivalence(std::string_view description, const T& value, S&& s, U&&... u)
    {
      return unit_testing::check_weak_equivalence(description, logger(), value, std::forward<S>(s), std::forward<U>(u)...);
    }

    bool check(std::string_view description, const bool value)
    {
      return unit_testing::check(description, logger(), value);
    }

    template<class E, class Fn>
    bool check_exception_thrown(std::string_view description, Fn&& function)
    {
      return unit_testing::check_exception_thrown<E>(description, logger(), std::forward<Fn>(function));
    }

    template<class Iter, class PredictionIter>
    bool check_range(std::string_view description, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return unit_testing::check_range(description, logger(), first, last, predictionFirst, predictionLast);
    }

    template<class Stream>
    friend Stream& operator<<(Stream& os, const checker& checker)
    {
      os << checker.logger();
      return os;
    }

    [[nodiscard]]
    log_summary summary(std::string_view prefix, const log_summary::duration delta) const
    {
      return log_summary{prefix, logger(), delta};
    }
  protected:
    using sentinel = typename unit_test_logger<Mode>::sentinel;
    
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
    sentinel make_sentinel(std::string_view message)
    {
      return {logger(), message};
    }
  private:
    [[nodiscard]]
    unit_test_logger<Mode>& logger() noexcept
    {
      return static_cast<unit_test_logger<Mode>&>(*this);
    }

    [[nodiscard]]
    const unit_test_logger<Mode>& logger() const noexcept 
    {
      return static_cast<const unit_test_logger<Mode>&>(*this);
    }
  };
}
