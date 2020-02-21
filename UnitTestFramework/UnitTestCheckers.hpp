////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestCheckers.hpp
    \brief Utilties for performing checks within the unit testing framework.
*/

#include "UnitTestCheckersDetails.hpp"

#include "ArrayUtilities.hpp"
#include "StatisticalAlgorithms.hpp"
#include "Utilities.hpp"

#include <chrono>
#include <random>
#include <future>
#include <utility>
#include <set>
#include <variant>

namespace sequoia
{
  namespace unit_testing
  {
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
      static_assert(is_serializable_v<T>);
      
      [[nodiscard]]
      static std::string make(const T& val)
      {        
        std::ostringstream os{};
        os << std::boolalpha << val;
        return os.str();
      }
    };

    template<class T, class=std::void_t<>>
    struct has_string_maker : std::false_type {};

    template<class T>
    struct has_string_maker<T, std::void_t<decltype(string_maker<T>{})>> : std::true_type {};

    template<class T>
    constexpr bool has_string_maker_v{has_string_maker<T>::value};

    template<class T>
    [[nodiscard]] std::string to_string(const T& value)
    {
      if constexpr(has_string_maker_v<T>)
      {
        return string_maker<T>::make(value);
      }
      else
      {
        // Use this mechanism to generate type-info in the error message
        static_assert(dependent_false<T>::value, "Type is not serializable");
      }
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

    template<class Logger, class Iter, class PredictionIter>
    bool check_range(std::string_view description, Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast);

    template<class Logger, class T>
    bool check(std::string_view description, Logger& logger, equality_tag, const T& value, const T& prediction)
    {
      constexpr bool delegate{has_detailed_equality_checker_v<T> || (is_container_v<T> && !is_serializable_v<T>)};

      static_assert(delegate || is_serializable_v<T> || is_equal_to_comparable_v<T>,
                    "Provide either a specialization of detailed_equality_checker or string_maker and/or operator==");
      
      using sentinel = typename Logger::sentinel;      
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
    
    template<class Logger, class T, class S, class... U>
    bool check(std::string_view description, Logger& logger, equivalence_tag, const T& value, S&& s, U&&... u)
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

    template<class Logger, class T, class S, class... U>
    bool check(std::string_view description, Logger& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u)
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
    
    template<class Logger, class T> bool check_equality(std::string_view description, Logger& logger, const T& value, const T& prediction)
    {
      return check(description, logger, equality_tag{}, value, prediction);
    }

    template<class Logger, class T, class S, class... U>
    bool check_equivalence(std::string_view description, Logger& logger, const T& value, S&& s, U&&... u)
    {
      return check(description, logger, equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)...);      
    }

    template<class Logger, class T, class S, class... U>
    bool check_weak_equivalence(std::string_view description, Logger& logger, const T& value, S&& s, U&&... u)
    {
      return check(description, logger, weak_equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)...);      
    }

    template<class Logger> bool check(std::string_view description, Logger& logger, const bool value)
    {
      return check_equality(description, logger, value, true);
    }

    template<class E, class Logger, class Fn>
    bool check_exception_thrown(std::string_view description, Logger& logger, Fn&& function)
    {
      const std::string message{"\t" + add_type_info<E>(combine_messages(description, "Expected Exception Type:", "\n"))};
      typename Logger::sentinel r{logger, message};
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
    
    template<class Logger, class Iter, class PredictionIter>
    bool check_range(std::string_view description, Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return impl::check_range(description, logger, equality_tag{}, first, last, predictionFirst, predictionLast);      
    }

    template<class Logger, class Iter, class PredictionIter>
    bool check_range_equivalence(std::string_view description, Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return impl::check_range(description, logger, equivalence_tag{}, first, last, predictionFirst, predictionLast);      
    }

    template<class Logger, class Iter, class PredictionIter>
    bool check_range_weak_equivalence(std::string_view description, Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
    {
      return impl::check_range(description, logger, weak_equivalence_tag{}, first, last, predictionFirst, predictionLast);      
    }

    struct individual_allocation_predictions
    {
      individual_allocation_predictions(int copyPrediction, int mutationPrediction)
        : copy{copyPrediction}          
        , mutation{mutationPrediction}
        , para_copy{copyPrediction}
        , para_move{copyPrediction}
      {}

      individual_allocation_predictions(int copyPrediction, int mutationPrediction, int copyLikePrediction)
        : copy{copyPrediction}          
        , mutation{mutationPrediction}
        , para_copy{copyLikePrediction}
        , para_move{copyPrediction}
      {}
      
      individual_allocation_predictions(int copyPrediction, int mutationPrediction, int copyLikePrediction, int moveLikePrediction)
        : copy{copyPrediction}          
        , mutation{mutationPrediction}
        , para_copy{copyLikePrediction}
        , para_move{moveLikePrediction}
      {}
      
      int copy{}, mutation{}, para_copy{}, para_move{};
    };

    struct assignment_allocation_predictions
    {
      assignment_allocation_predictions(int withPropagation, int withoutPropagation)
        : with_propagation{withPropagation}, without_propagation{withoutPropagation}
      {}
      
      int with_propagation{}, without_propagation{};
    };
 
    struct allocation_predictions
    {
      allocation_predictions(int copyX, individual_allocation_predictions yPredictions, assignment_allocation_predictions assignYtoX)
        : copy_x{copyX}, y{yPredictions}, assign_y_to_x{assignYtoX}
      {}
      
      int copy_x{};
      individual_allocation_predictions y;
      assignment_allocation_predictions assign_y_to_x;
    };

    struct move_only_allocation_predictions
    {
      int para_move{};
    };

    template<class Container, class Allocator, class Predictions>
    class allocation_info : public impl::allocation_info_base<Container, Allocator>
    {
    private:
      using base_t = impl::allocation_info_base<Container, Allocator>;
    public:
      using container_type   = Container;
      using allocator_type   = Allocator;
      using predictions_type = Predictions;
      
      template<class Fn>
      allocation_info(Fn&& allocGetter, Predictions predictions)
        : base_t{std::forward<Fn>(allocGetter)}
        , m_Predictions{std::move(predictions)}
      {}

      [[nodiscard]]
      const Predictions& get_predictions() const noexcept
      {
        return m_Predictions;
      }
    private:
      Predictions m_Predictions;
    };

    template<class Container, class... Allocators, class Predictions>
    class allocation_info<Container, std::scoped_allocator_adaptor<Allocators...>, Predictions>
      : public impl::allocation_info_base<Container, std::scoped_allocator_adaptor<Allocators...>>
    {
    private:
      using base_t = impl::allocation_info_base<Container, std::scoped_allocator_adaptor<Allocators...>>;
    public:
      constexpr static auto N{sizeof...(Allocators)};

      using container_type   = Container;
      using allocator_type   = std::scoped_allocator_adaptor<Allocators...>;
      using predictions_type = Predictions;

      template<class Fn>
      allocation_info(Fn&& allocGetter, std::initializer_list<Predictions> predictions)
        : base_t{std::forward<Fn>(allocGetter)}
        , m_Predictions{utilities::to_array<Predictions, N>(predictions)}
      {}

      template<std::size_t I> decltype(auto) unpack() const
      {
        if constexpr(I==0)
        {
          return *this;
        }
        else
        {
          auto scopedGetter{[getter=this->make_getter()](const Container& c){
              return get<I>(getter(c));
            }
          };

          using Alloc = decltype(scopedGetter(std::declval<Container>()));

          return allocation_info<Container, Alloc>{scopedGetter, m_Predictions[I]};
        }
      }

      [[nodiscard]]
      const allocation_predictions& get_predictions() const noexcept
      {
        return m_Predictions[0];
      }
    private:
      template<std::size_t I, class... As>
      static auto get(const std::scoped_allocator_adaptor<As...>& alloc)
      {
        if constexpr(I==0)
        {
          return alloc.outer_allocator();
        }
        else
        {
          return get<I-1>(alloc.inner_allocator());
        }
      }
      
      std::array<Predictions, N> m_Predictions;
    };

    template<class Fn>
    allocation_info(Fn&& allocGetter, allocation_predictions predictions)
      -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

    template<class Fn>
    allocation_info(Fn&& allocGetter, std::initializer_list<allocation_predictions> predictions)
      -> allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

    // Done through inheritance rather than a using declaration
    // in order to make use of CTAD. A shame argument deduction
    // can't be used for using declarations...
    
    template<class Container, class Allocator>
    class move_only_allocation_info
      : public allocation_info<Container, Allocator, move_only_allocation_predictions>
    {
    public:
      using allocation_info<Container, Allocator, move_only_allocation_predictions>::allocation_info;
    };

    template<class Fn>
    move_only_allocation_info(Fn&& allocGetter, move_only_allocation_predictions predictions)
      -> move_only_allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

    template<class Fn>
    move_only_allocation_info(Fn&& allocGetter, std::initializer_list<move_only_allocation_predictions> predictions)
      -> move_only_allocation_info<std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::arg>, std::decay_t<typename function_signature<decltype(&std::decay_t<Fn>::operator())>::ret>>;

    template<class Logger, class T, class Mutator, class... Allocators>
    void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<T, Allocators>... info)
    {
      typename Logger::sentinel s{logger, add_type_info<T>(description)};
      
      if(impl::check_regular_semantics(description, logger, impl::regular_allocation_actions{}, x, y, yMutator, std::tuple_cat(impl::make_allocation_checkers(info, x, y)...)))
      {
        impl::check_para_constructor_allocations(description, logger, y, yMutator, info...);
      }
    }

    template<class Logger, class T, class Mutator>
    void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator)
    {
      impl::check_regular_semantics(description, logger, impl::regular_default_actions{}, x, y, yMutator);
    }
    
    template<class Logger, class T>
    void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y)
    {
      impl::check_regular_semantics(description, logger, impl::regular_default_actions{}, x, y, impl::null_mutator{});
    }

    template<class Logger, class T, class... Allocators>
    void check_regular_semantics(std::string_view description, Logger& logger, T&& x, T&& y, const T& xClone, const T& yClone, move_only_allocation_info<T, Allocators>... info)
    {
      typename Logger::sentinel s{logger, add_type_info<T>(description)};

      if(impl::check_regular_semantics(description, logger, impl::regular_allocation_actions{}, std::forward<T>(x), std::forward<T>(y), xClone, yClone, std::tuple_cat(impl::make_allocation_checkers(info, x, y)...)))
      {
        impl::check_para_constructor_allocations(description, logger, std::forward<T>(y), yClone, info...);
      }
    }

    template<class Logger, class T, class... Allocators>
    void check_regular_semantics(std::string_view description, Logger& logger, T&& x, T&& y, const T& xClone, const T& yClone)
    {
      impl::check_regular_semantics(description, logger, impl::regular_default_actions{}, std::forward<T>(x), std::forward<T>(y), xClone, yClone);
    }

    template<class Logger, class... Extenders>
    class checker : private Logger, public Extenders...
    {
    public:
      static_assert(((sizeof(Extenders) == sizeof(Logger*)) && ...),
                    "The state of any Extenders must comprise precisely a reference to a Logger");
      
      constexpr static test_mode mode{Logger::mode};
      
      checker() : Extenders{logger()}... {}
      
      checker(const checker&)            = delete;
      checker& operator=(const checker&) = delete;      
      checker& operator=(checker&&)      = delete;

      template<class T> bool check_equality(std::string_view description, const T& value, const T& prediction)
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

      log_summary summary(std::string_view prefix, const log_summary::duration delta) const
      {
        return log_summary{prefix, logger(), delta};
      }
    protected:
      checker(checker&& other) noexcept
        : Logger{static_cast<Logger&&>(other)}
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

      typename Logger::sentinel make_sentinel(std::string_view message)
      {
        return {logger(), message};
      }
    private:
      Logger& logger()
      {
        return static_cast<Logger&>(*this);
      }

      const Logger& logger() const
      {
        return static_cast<const Logger&>(*this);
      }
    };
  }
}
