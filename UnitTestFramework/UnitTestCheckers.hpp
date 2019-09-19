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

#include "UnitTestLogger.hpp"

#include "StatisticalAlgorithms.hpp"

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
    template<class T, class=std::void_t<>> struct is_serializable : public std::false_type
    {};

    // Seems to be a bug here for clang-1000.11.45.5 since decltype returns std::stringstream, whatever T!
    template<class T> struct is_serializable<T, std::void_t<decltype(std::stringstream{} << std::declval<T>())>> : public std::true_type
    {};

    template<class T> constexpr bool is_serializable_v{is_serializable<T>::value};

    template<class T, class=std::enable_if_t<is_serializable_v<T>>>
    struct string_maker
    {
      [[nodiscard]]
      static std::string make(const T& val)
      {        
        std::ostringstream os;
        os << std::boolalpha << val;
        return os.str();
      }
    };

    template<class T> [[nodiscard]] std::string to_string(const T& value)
    {
      return string_maker<T>::make(value);
    }

    template<class T> struct detailed_equality_checker;

    template<class T> struct equivalence_checker;

    template<class T> struct weak_equivalence_checker;

    template<class T, class=std::void_t<>> struct has_detailed_equality_checker : public std::false_type
    {};

    template<class T> struct has_detailed_equality_checker<T, std::void_t<decltype(detailed_equality_checker<T>{})>> : public std::true_type
    {};

    template<class T> constexpr bool has_detailed_equality_checker_v{has_detailed_equality_checker<T>::value};

    struct equality_tag{};
    struct equivalence_tag{};
    struct weak_equivalence_tag{};

    template<class T, class... U> std::string make_type_info()
    {        
      std::string info{demangle<T>()};
      if constexpr(sizeof...(U) > 0)
        info.append("\n;").append(make_type_info<U...>());

      return info;
    }

    template<class T, class... U> std::string add_type_info(std::string_view description)
    {
      return combine_messages(description, "[" + make_type_info<T, U...>() + "]\n",
                              description.empty() ? "" : (description.back() == '\n') ? "\n" : "\n\n");
    }
    
    namespace impl
    {           
      template<class EquivChecker, class Logger, class T, class S, class... U>
      bool check(std::string_view description, Logger& logger, const T& value, const S& s, const U&... u)
      {
        using sentinel = typename Logger::sentinel;

        const std::string message{
          add_type_info<S, U...>(
            combine_messages(description, "Comparison performed using:\n\t[" + demangle<EquivChecker>() + "]\n\tWith equivalent types:", "\n"))
        };
      
        sentinel r{logger, message};
        const auto previousFailures{logger.failures()};

        EquivChecker::check(message, logger, value, s, u...);
      
        return logger.failures() == previousFailures;
      }

      template<class... Allocators>
      constexpr bool do_swap() noexcept
      {
        return ((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
                 || std::allocator_traits<Allocators>::is_always_equal::value) && ... );
      }
    }

    template<class Logger, class Iter, class PredictionIter>
    bool check_range(std::string_view description, Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast);

    template<class Logger, class T>
    bool check(std::string_view description, Logger& logger, equality_tag, const T& value, const T& prediction)
    {
      constexpr bool delegate{has_detailed_equality_checker_v<T> || is_container_v<T>};
       
      static_assert(delegate || is_serializable_v<T> || is_equal_to_comparable_v<T>,
                    "Provide either a specialization of detailed_equality_checker or string_maker and/or operator==");
      
      using sentinel = typename Logger::sentinel;

      const auto priorFailures{logger.failures()};
      
      auto messageGenerator{
        [description](std::string op, std::string retVal){
          const std::string info{"\t[" + demangle<T>() + "]\n\toperator" + std::move(op) + " returned " + std::move(retVal) + "\n"};            
          return description.empty() ? info : std::string{"\t"}.append(description).append("\n" + info);
        }
      };
      
      if constexpr(is_equal_to_comparable_v<T>)
      {
        sentinel s{logger, add_type_info<T>(description)};
        s.log_check();
        if(!(prediction == value))
        {
          auto message{messageGenerator("==", "false")};
          if constexpr(!delegate)
          {
            message.append("\t" + to_string(value) +  " differs from " + to_string(prediction) + "\n\n");
          }
          logger.log_failure(message);
        }
      }

      if constexpr(delegate)
      {
        if constexpr(is_not_equal_to_comparable_v<T>)
        {
          sentinel s{logger, add_type_info<T>(description)};
          s.log_check();
          if(prediction != value)
          {
            logger.log_failure(messageGenerator("!=", "true"));
          }
        }

        if constexpr(has_detailed_equality_checker_v<T>)
        {          
          sentinel s{logger, add_type_info<T>(description)};
          detailed_equality_checker<T>::check(description, logger, value, prediction);
        }
        else if constexpr(is_container_v<T>)
        {
          check_range(description, logger, std::begin(value), std::end(value), std::begin(prediction), std::end(prediction));
        }      
        else
        {
          static_assert(dependent_false<T>::value, "Unable to check the details for input type");
        }
      }

      return logger.failures() == priorFailures;
    }
    
    template<class Logger, class T, class S, class... U>
    bool check(std::string_view description, Logger& logger, equivalence_tag, const T& value, S&& s, U&&... u)
    {
      return impl::check<equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);      
    }

    template<class Logger, class T, class S, class... U>
    bool check(std::string_view description, Logger& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u)
    {
      return impl::check<weak_equivalence_checker<T>>(description, logger, value, std::forward<S>(s), std::forward<U>(u)...);
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

    template<class Logger, class T>
    bool check_equality_within_tolerance(std::string_view description, Logger& logger, const T& value, const T& prediction, const T& tol)
    {
      auto message{combine_messages(description, "Check using tolerance of " + std::to_string(tol))};
      return check(std::move(message), logger, (value > prediction - tol) && (value < prediction + tol));
    }

    template<class Logger, class E, class Fn>
    bool check_exception_thrown(std::string_view description, Logger& logger, Fn&& function)
    {
      typename Logger::sentinel r{logger, description};
      r.log_check();
      try
      {
        function();
        logger.log_failure(combine_messages(description, "\tNo exception thrown\n", "\n"));
        return false;
      }
      catch(E&)
      {
        return true;
      }
      catch(std::exception& e)
      {
        logger.log_failure(combine_messages(description, std::string{"\tUnexpected exception thrown -\""} + e.what() + "\"\n", "\n"));
        return false;
      }
      catch(...)
      {
        logger.log_failure(combine_messages(description, "\tUnknown exception thrown\n", "\n"));
        return false;
      }
    }

    namespace impl
    {
      template<class Logger, class Tag, class Iter, class PredictionIter>
      bool check_range(std::string_view description, Logger& logger, Tag tag, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
      {
        typename Logger::sentinel r{logger, description};
        bool equal{true};

        using std::distance;
        const auto predictedSize{distance(predictionFirst, predictionLast)};
        if(check_equality(combine_messages(description, "container size wrong"), logger, distance(first, last), predictedSize))
        {
          auto predictionIter{predictionFirst};
          auto iter{first};
          for(; predictionIter != predictionLast; ++predictionIter, ++iter)
          {
            std::string dist{std::to_string(std::distance(predictionFirst, predictionIter)).append("\n")};
            if(!check(combine_messages(description, "element ").append(std::move(dist)), logger, tag, *iter, *predictionIter)) equal = false;
          }
        }
        else
        {
          equal = false;
        }
      
        return equal;
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
 
    struct allocation_predictions
    {
      int copy_x{}, copy_y{}, copy_assign_y_to_x{}, mutation{}, move_like_x{};
    };
    
    enum class mutation_flavour {after_move_assign, after_swap};

    template<class Allocator>
    class allocation_info
    {
    public:      
      allocation_info(const Allocator& xAlloc, const Allocator& yAlloc, allocation_predictions predictions)
        : allocation_info{variant{xAlloc}, variant{yAlloc}, std::move(predictions)}
      {}

      template<class Fn>
      allocation_info(const Allocator& xAlloc, Fn yAllocGetter, allocation_predictions predictions)
        : allocation_info{variant{xAlloc}, variant{std::move(yAllocGetter)}, std::move(predictions)}
      {}

      template<class Fn>
      allocation_info(Fn xAllocGetter, const Allocator& yAlloc, allocation_predictions predictions)
        : allocation_info{variant{std::move(xAllocGetter)}, variant{yAlloc}, std::move(predictions)}
      {}

      template<class Fn>
      allocation_info(Fn xAllocGetter, Fn yAllocGetter, allocation_predictions predictions)
        : allocation_info{variant{std::move(xAllocGetter)}, variant{std::move(yAllocGetter)}, std::move(predictions)}
      {}

      template<class Logger>
      void check_copy_x(std::string_view description, Logger& logger)
      {
        auto message{combine_messages(description, "Copy construction allocation (x)")};
        m_xCurrentCount = check_construction(std::move(message), logger, m_xAllocator, m_xCurrentCount, m_Predictions.copy_x);
      }

      template<class Logger>
      void check_copy_y(std::string_view description, Logger& logger)
      {
        auto message{combine_messages(description, "Copy construction allocation (y)")};
        m_yCurrentCount = check_construction(std::move(message), logger, m_yAllocator, m_yCurrentCount, m_Predictions.copy_y);
      }

      template<class Logger>
      void check_move(std::string_view description, Logger& logger)
      {
        auto message{combine_messages(description, "Move construction allocation")};
        m_yCurrentCount = check_construction(std::move(message), logger, m_yAllocator, m_yCurrentCount, 0);
      }

      template<class Logger>
      void check_copy_assign_y_to_x(std::string_view description, Logger& logger)
      {
        typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};
        
        int xPrediction{}, yPrediction{};
        if constexpr(std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
        {
          yPrediction = m_Predictions.copy_assign_y_to_x;
        }
        else
        {
          xPrediction = m_Predictions.copy_assign_y_to_x;
        }

        check(description, "Copy assignment x allocations", "Copy assignment y allocations", logger, xPrediction, yPrediction);
      }

      template<class Logger>
      void check_move_assign_y_to_x(std::string_view description, Logger& logger)
      {
        typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

        const bool copyLike{!std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
            && (get(m_xAllocator) != get(m_yAllocator))};
        
        const int xPrediction{copyLike ? m_Predictions.copy_assign_y_to_x : 0}, yPrediction{};

        check(description, "Move assignment x allocations", "Move assignment y allocations", logger, xPrediction, yPrediction);
     }

      template<mutation_flavour Flavour, class Logger>
      void check_mutation(std::string_view description, Logger& logger)
      {
        typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};

        int xPrediction{}, yPrediction{};
        constexpr bool pred{[](){
            switch(Flavour)
            {
            case mutation_flavour::after_move_assign:
              return std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value;
            case mutation_flavour::after_swap:
              return std::allocator_traits<Allocator>::propagate_on_container_swap::value;
            }
          }()
        };
        
        if constexpr(pred)
        {
          yPrediction = m_Predictions.mutation;
        }
        else
        {
          xPrediction = m_Predictions.mutation;
        }

        check(description, "Mutation x allocations", "Mutation y allocations", logger, xPrediction, yPrediction);
      }

      [[nodiscard]]
      const allocation_predictions& predictions() const noexcept { return m_Predictions; }
    private:
      using variant = std::variant<Allocator, std::function<Allocator()>>;

      struct alloc_getter
      {        
        Allocator operator()(const Allocator& alloc) const
        {
          return alloc;
        }

        Allocator operator()(const std::function<Allocator()>& fn) const
        {
          return fn();
        }
      };

      static Allocator get(const variant& v)
      {
        return std::visit(alloc_getter{}, v);
      }

      static int count(const variant& v)
      {
        return std::visit(alloc_getter{}, v).allocs();
      }
      
      variant m_xAllocator, m_yAllocator;
      
      allocation_predictions m_Predictions;
      
      int m_xCurrentCount{}, m_yCurrentCount{};

      allocation_info(variant xAlloc, variant yAlloc, allocation_predictions predictions)
        : m_xAllocator{std::move(xAlloc)}
        , m_yAllocator{std::move(yAlloc)}
        , m_Predictions{std::move(predictions)}
        , m_xCurrentCount{count(m_xAllocator)}
        , m_yCurrentCount{count(m_yAllocator)}
      {} 

      template<class Logger>
      static int check_construction(std::string description, Logger& logger, const variant& v, const int currentCount, const int deltaPrediction)
      {
        typename Logger::sentinel s{logger, add_type_info<Allocator>(description)};
        const int current{count(v)};
        
        check_equality(std::move(description), logger, current - currentCount, deltaPrediction);

        return current;
      }

      template<class Logger>
      void check(std::string_view description, std::string_view xMessage, std::string_view yMessage, Logger& logger, const int xDeltaPrediction, const int yDeltaPrediction)
      {
        const int xCurrent{count(m_xAllocator)}, yCurrent{count(m_yAllocator)};
        
        const int xDeltaActual{xCurrent - m_xCurrentCount};
        const int yDeltaActual{yCurrent - m_yCurrentCount};
        
        m_xCurrentCount = xCurrent;
        m_yCurrentCount = yCurrent;

        check_equality(combine_messages(description, xMessage), logger, xDeltaActual, xDeltaPrediction);
        check_equality(combine_messages(description, yMessage), logger, yDeltaActual, yDeltaPrediction);
      }
    };

    namespace impl
    {
      template<class Logger, class Check, class Allocator, class... Allocators>
      void check_allocation(std::string_view description, Logger& logger, Check check, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        check(add_type_info<Allocator>(description), allocationInfo);

        if constexpr (sizeof...(Allocators) > 0)
        {
          check_allocation(description, logger, check, moreInfo...); 
        }
      }

      template<class Logger, class Check, class... AllAllocators, class Allocator, class... Allocators>
      void check_allocation(std::string_view description, Logger& logger, Check check, const std::tuple<AllAllocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        check(add_type_info<Allocator>(description), std::get<sizeof...(AllAllocators)-1-sizeof...(Allocators)>(allocators), allocationInfo);

        if constexpr (sizeof...(Allocators) > 0)
        {
          check_allocation(description, logger, check, allocators, moreInfo...); 
        }
      }
      
      template<class Logger, class Allocator, class... Allocators>
      void check_copy_x_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, auto& info){
            info.check_copy_x(message, logger);
          }
        };

        check_allocation(description, logger, checker, allocationInfo, moreInfo...);
      }

      template<class Logger, class Allocator, class... Allocators>
      void check_copy_y_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, auto& info){
            info.check_copy_y(message, logger);
          }
        };

        check_allocation(description, logger, checker, allocationInfo, moreInfo...);
      }

      template<class Logger, class Allocator, class... Allocators>
      void check_move_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, auto& info){
            info.check_move(message, logger);
          }
        };

        check_allocation(description, logger, checker, allocationInfo, moreInfo...);
      }
      
      template<class Logger, class Allocator, class... Allocators>
      void check_copy_assign_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, auto& info){
            info.check_copy_assign_y_to_x(message, logger);
          }
        };

        check_allocation(description, logger, checker, allocationInfo, moreInfo...);
      }

      template<class Logger, class Allocator, class... Allocators>
      void check_move_assign_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, auto& info){
            info.check_move_assign_y_to_x(message, logger);
          }
        };

        check_allocation(description, logger, checker, allocationInfo, moreInfo...);
      }

      template<mutation_flavour Flavour, class Logger, class Allocator, class... Allocators>
      void check_mutation_allocation(std::string_view description, Logger& logger, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, auto& info){
            info.template check_mutation<Flavour>(message, logger);
          }
        };

        check_allocation(description, logger, checker, allocationInfo, moreInfo...);
      }

      template<class Logger, class Allocator, class... Allocators>
      void check_copy_like_x_allocation(std::string_view description, Logger& logger, const std::tuple<Allocator, Allocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, const auto& alloc, const auto& info){
            check_equality(message, logger, alloc.allocs(), info.predictions().copy_x);
          }
        };

        check_allocation(description, logger, checker, allocators, allocationInfo, moreInfo...);
      }

      template<class Logger, class Allocator, class... Allocators>
      void check_move_like_x_allocation(std::string_view description, Logger& logger, const std::tuple<Allocator, Allocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, const auto& alloc, const auto& info){
            check_equality(message, logger, alloc.allocs(), info.predictions().move_like_x);
          }
        };

        check_allocation(description, logger, checker, allocators, allocationInfo, moreInfo...);
      }

      template<class Logger, class Allocator, class... Allocators>
      void check_mutation_allocation(std::string_view description, Logger& logger, const std::tuple<Allocator, Allocators...>& allocators, allocation_info<Allocator>& allocationInfo, allocation_info<Allocators>&... moreInfo)
      {
        auto checker{
          [&logger](std::string_view message, const auto& alloc, const auto& info){
            check_equality(message, logger, alloc.allocs(), info.predictions().mutation + info.predictions().move_like_x);
          }
        };

        check_allocation(description, logger, checker, allocators, allocationInfo, moreInfo...);
      }

      struct null_mutator
      {
        template<class T> void operator()(T&) noexcept {};
      };

      template<class Logger, class T, class Mutator, class... Allocators, std::size_t... I>
      void check_allocations(std::index_sequence<I...>, std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<Allocators>... allocationInfo)
      {
        {
          T u{x}, v{y};
          check_copy_x_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);
          check_copy_y_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);

          u = std::move(v);
          check_move_assign_allocation(combine_messages(description, "Move assign allocations"), logger, allocationInfo...);

          yMutator(u);
          check_mutation_allocation<mutation_flavour::after_move_assign>(combine_messages(description, "mutation after move assignment allocations"), logger, allocationInfo...);
          }

        if constexpr(((   std::allocator_traits<Allocators>::propagate_on_container_swap::value
                       || std::allocator_traits<Allocators>::is_always_equal::value) && ...))
        {
          T u{x}, v{y};
          check_copy_x_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);
          check_copy_y_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);

          using std::swap;
          swap(u, v);

          yMutator(u);
          check_mutation_allocation<mutation_flavour::after_swap>(combine_messages(description, "mutation after swap allocations"), logger, allocationInfo...);
        }

        {
          auto make{
            [description, &logger, &x](auto&... info){
              std::tuple<Allocators...> allocs{};

              T u{x, std::get<I>(allocs)...};
              check_equality(combine_messages(description, "Copy-like construction"), logger, u, x);
              check_copy_like_x_allocation(combine_messages(description, "Copy-like allocations"), logger, allocs, info...);

              return u;
            }
          };

          std::tuple<Allocators...> allocs{};

          T v{make(allocationInfo...), std::get<I>(allocs)...};
          check_equality(combine_messages(description, "Move-like construction"), logger, v, x);
          check_move_like_x_allocation(combine_messages(description, "Move-like allocations"), logger, allocs, allocationInfo...);

          yMutator(v);
          check_mutation_allocation(combine_messages(description, "mutation allocations after move-like construction"), logger, allocs, allocationInfo...);
        }
      }

      template<class Logger, class T, class Mutator, class... Allocators>
      void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<Allocators>... allocationInfo)
      {
        constexpr bool nullMutator{std::is_same_v<Mutator, null_mutator>};
        constexpr bool checkAllocs{sizeof...(Allocators) > 0};
        static_assert(!checkAllocs || !nullMutator);
        
        typename Logger::sentinel s{logger, add_type_info<T>(description)};

        if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x)) return;
        if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x))) return;

        if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y)) return;

        T z{x};
        check_equality(combine_messages(description, "Copy constructor"), logger, z, x);
        check(combine_messages(description, "Equality operator"), logger, z == x);
        if constexpr(checkAllocs)
          check_copy_x_allocation(combine_messages(description, "Copy allocations"), logger, allocationInfo...);
      
        z = y;
        check_equality(combine_messages(description, "Copy assignment"), logger, z, y);
        check(combine_messages(description, "Inequality operator"), logger, z != x);
        if constexpr(checkAllocs)
          check_copy_assign_allocation(combine_messages(description, "Copy assign allocations"), logger, allocationInfo...);

        T w{std::move(z)};
        check_equality(combine_messages(description, "Move constructor"), logger, w, y);
        if constexpr(checkAllocs)
          check_move_allocation(combine_messages(description, "Move allocations"), logger, allocationInfo...);

        if constexpr(checkAllocs)
        {
          check_allocations(std::make_index_sequence<sizeof...(Allocators)>{}, description, logger, x, y, yMutator, allocationInfo...);
        }

        z = T{x};
        check_equality(combine_messages(description, "Move assignment"), logger, z, x);

        if constexpr(!nullMutator)
        {
          T v{y};
          yMutator(v);
          check(combine_messages(description, "Copy constructor does not have value semantics"), logger, y == y);
          check(combine_messages(description, "Mutation is not doing anything following copy constrution"), logger, v != y);

          v = y;
          check_equality(combine_messages(description, "Copy assignment"), logger, v, y);

          yMutator(v);
          check(combine_messages(description, "Copy assignment does not have value semantics"), logger, y == y);
          check(combine_messages(description, "Mutation is not doing anything following copy assignment"), logger, v != y);
        }
      }
    }

    template<class Logger, class T, class Mutator, class... Allocators>
    void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y, Mutator yMutator, allocation_info<Allocators>... allocationInfo)
    {
      impl::check_regular_semantics(description, logger, x, y, yMutator, allocationInfo...);
    }

    template<class Logger, class T>
    void check_regular_semantics(std::string_view description, Logger& logger, const T& x, const T& y)
    {
      impl::check_regular_semantics(description, logger, x, y, impl::null_mutator{});
    }

    template<class Logger, class T, class... Allocators>
    void check_regular_semantics(std::string_view description, Logger& logger, T&& x, T&& y, const T& xClone, const T& yClone, const Allocators&... allocators)
    {
      typename Logger::sentinel s{logger, add_type_info<T>(description)};

      if(!check(combine_messages(description, "Equality operator is inconsistent"), logger, x == x)) return;
      if(!check(combine_messages(description, "Inequality operator is inconsistent"), logger, !(x != x))) return;

      if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and y are assumed to be different"), logger, x != y)) return;

      if(!check(combine_messages(description, "Precondition - for checking regular semantics, x and xClone are assumed to be equal"), logger, x == xClone)) return;

      if(!check(combine_messages(description, "Precondition - for checking regular semantics, y and yClone are assumed to be equal"), logger, y == yClone)) return;

      T z{std::move(x)};
      check_equality(combine_messages(description, "Move constructor"), logger, z, xClone);
        
      x = std::move(y);
      check_equality(combine_messages(description, "Move assignment"), logger, x, yClone);

      if constexpr (impl::do_swap<Allocators...>())
      {
        using std::swap;
        swap(z, x);
        check_equality(combine_messages(description, "Swap"), logger, x, xClone);
        check_equality(combine_messages(description, "Swap"), logger, z, yClone);
      }

      if constexpr(sizeof...(allocators) > 0)
      {
        T u{std::move(x), allocators...};
        check_equality(combine_messages(description, "Move constructor using allocator"), logger, u, xClone);
      }
    }
        
    template<class Logger, class T, class Mutator>
    void check_copy_consistency(std::string_view description, Logger& logger, T& target, const T& prediction, Mutator m)
    {
      typename Logger::sentinel s{logger, add_type_info<T>(description)};

      auto c{target};
      
      m(target);
      check_equality(combine_messages(description, "Mutation of target"), logger, target, prediction);

      m(c);
      check_equality(combine_messages(description, "Mutation of copy"), logger, c, prediction);
      
    }

    template<class T, class S>
    struct detailed_equality_checker<std::pair<T, S>>
    {
      template<class Logger>
      static void check(std::string_view description, Logger& logger, const std::pair<T, S>& value, const std::pair<T,S>& prediction)
      {
        check_equality(combine_messages(description, "First element of pair is incorrent"), logger, value.first, prediction.first);
        check_equality(combine_messages(description, "First element of pair is incorrect"), logger, value.second, prediction.second);
      }
    };

    template<class... T>
    struct detailed_equality_checker<std::tuple<T...>>
    {     
    private:
      template<class Logger, std::size_t I = 0>
      static void check_tuple_elements(std::string_view description, Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction)
      {
        if constexpr(I < sizeof...(T))
        {
          const std::string message{std::to_string(I) + "th element of tuple incorrect"};
          check_equality(combine_messages(description, message), logger, std::get<I>(value), std::get<I>(prediction));
          check_tuple_elements<Logger, I+1>(description, logger, value, prediction);
        }
      }
        
    public:
      template<class Logger>
      static void check(std::string_view description, Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction)
      {
        check_tuple_elements(description, logger, value, prediction);
      }
    };
    
    template<class R> struct performance_results
    {
      std::vector<std::future<R>> fast_futures, slow_futures;
      bool passed{};
    };

    template<class Logger, class F, class S>
    auto check_relative_performance(std::string_view description, Logger& logger, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials, const double num_sds) -> performance_results<std::invoke_result_t<F>>
    {      
      using R = std::invoke_result_t<F>;
      static_assert(std::is_same_v<R, std::invoke_result_t<S>>, "Fast/Slow invokables must have same return value");
            
      // Replace with contracts in 2020
      if((minSpeedUp <= 1) || (maxSpeedUp <= 1))
        throw std::logic_error("Relative performance test requires speed-up factors > 1!");

      if(minSpeedUp > maxSpeedUp)
        throw std::logic_error("maxSpeedUp must be >= minSpeedUp");      
      
      performance_results<R> results;      
      
      using namespace std::chrono;
      using namespace maths;

      std::vector<double> fastData, slowData;
      fastData.reserve(trials);
      slowData.reserve(trials);
      
      std::random_device generator;
      for(std::size_t i{}; i < trials; ++i)
      {
        std::packaged_task<R()> fastTask{fast}, slowTask{slow};

        results.fast_futures.emplace_back(std::move(fastTask.get_future()));
        results.slow_futures.emplace_back(std::move(slowTask.get_future()));

        steady_clock::time_point start, end;

        std::uniform_real_distribution<double> distribution{0.0, 1.0};
        const bool fastFirst{(distribution(generator) < 0.5)};

        start = steady_clock::now();
        fastFirst ? fastTask() : slowTask();
        end = steady_clock::now();

        duration<double> duration = end - start;
        fastFirst ? fastData.push_back(duration.count()) : slowData.push_back(duration.count());

        start = steady_clock::now();
        fastFirst ? slowTask() : fastTask();
        end = steady_clock::now();

        duration = end - start;
        fastFirst ? slowData.push_back(duration.count()) : fastData.push_back(duration.count());
      }

      auto compute_stats{
        [](auto first, auto last) {
          const auto data{sample_standard_deviation(first, last)};
          return std::make_pair(data.first.value(), data.second.value());
        }
      };
      
      const auto [sig_f, m_f]{compute_stats(fastData.cbegin(), fastData.cend())};
      const auto [sig_s, m_s]{compute_stats(slowData.cbegin(), slowData.cend())};

      using namespace maths::bias;
      if(m_f + sig_f < m_s - sig_s)
      {
        if(sig_f >= sig_s)
        {
          results.passed = (minSpeedUp * m_f <= (m_s + num_sds * sig_s))
                        && (maxSpeedUp * m_f >= (m_s - num_sds * sig_s));
        }
        else
        {
          results.passed = (m_s / maxSpeedUp <= (m_f + num_sds * sig_f))
                        && (m_s / minSpeedUp >= (m_f - num_sds * sig_f));
        }
      }

      auto serializer{
        [m_f=m_f,sig_f=sig_f,m_s=m_s,sig_s=sig_s,num_sds,minSpeedUp,maxSpeedUp](){
          std::ostringstream message;
          message << "\tFast Task duration: " << m_f << "s";
          message << " +- " << num_sds << " * " << sig_f;
          message << "\n\tSlow Task duration: " << m_s << "s";
          message << " +- " << num_sds << " * " << sig_s;
          message << " [" << m_s / m_f << "; (" << minSpeedUp << ", " << maxSpeedUp << ")]\n";

          return message.str();
        }
      };

      const std::string message{description.empty() ? serializer() : std::string{"\t"}.append(description).append("\n" + serializer())};
      
      typename Logger::sentinel r{logger, message};
      r.log_performance_check();

      if(!results.passed)
      {        
        logger.log_performance_failure(message);
      }

      return results;
    }
    

    template<class Logger>
    class checker
    {
    public:
      constexpr static test_mode mode{Logger::mode};
      
      checker() = default;
      
      checker(const checker&)            = delete;
      checker& operator=(const checker&) = delete;

      template<class T> bool check_equality(std::string_view description, const T& value, const T& prediction)
      {
        return unit_testing::check_equality(description, m_Logger, value, prediction);
      }

      template<class T, class S, class... U>
      bool check_equivalence(std::string_view description, const T& value, S&& s, U&&... u)
      {
        return unit_testing::check_equivalence(description, m_Logger, value, std::forward<S>(s), std::forward<U>(u)...);
      }

      template<class T, class S, class... U>
      bool check_weak_equivalence(std::string_view description, const T& value, S&& s, U&&... u)
      {
        return unit_testing::check_weak_equivalence(description, m_Logger, value, std::forward<S>(s), std::forward<U>(u)...);
      }
      
      template<class T>
      bool check_equality_within_tolerance(std::string_view description, const T prediction, const T value, const T tol)
      {
        return unit_testing::check_equality_within_tolerance(description, m_Logger, value, prediction, tol);
      }

      bool check(std::string_view description, const bool value)
      {
        return unit_testing::check(description, m_Logger, value);
      }

      template<class E, class Fn>
      bool check_exception_thrown(std::string_view description, Fn&& function)
      {
        return unit_testing::check_exception_thrown<Logger, E, Fn>(description, m_Logger, std::forward<Fn>(function));
      }

      template<class Iter, class PredictionIter>
      bool check_range(std::string_view description, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast)
      {
        return unit_testing::check_range(description, m_Logger, first, last, predictionFirst, predictionLast);
      }

      template<class T>
      void check_regular_semantics(std::string_view description, const T& x, const T& y)
      {
        unit_testing::check_regular_semantics(description, m_Logger, x, y);
      }

      template<class T, class Mutator>
      void check_regular_semantics(std::string_view description, const T& x, const T& y, Mutator m)
      {
        unit_testing::check_regular_semantics(description, m_Logger, x, y, m);
      }

      template<class T, class... Allocators>
      void check_regular_semantics(std::string_view description, T&& x, T&& y, const T& xClone, const T& yClone, const Allocators&... allocators)
      {
        unit_testing::check_regular_semantics(description, m_Logger, std::move(x), std::move(y), xClone, yClone, allocators...);
      }

      template<class T, class Mutator, class... Allocators>
      void check_regular_semantics(std::string_view description, const T& x, const T& y, Mutator m, allocation_info<Allocators>... allocationInfo)
      {
        unit_testing::check_regular_semantics(description, m_Logger, x, y, m, allocationInfo...);
      }

      // retire!
      template<class T, class Mutator>
      void check_copy_consistency(std::string_view description, T& target, const T& prediction, Mutator m)
      {
        unit_testing::check_copy_consistency(description, m_Logger, target, prediction, std::move(m));
      }
      
      template<class F, class S>
      auto check_relative_performance(std::string_view description, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials=5, const double num_sds=3)
        -> performance_results<decltype(fast())>
      {
        return unit_testing::check_relative_performance(description, m_Logger, fast, slow, minSpeedUp, maxSpeedUp, trials, num_sds);
      }

      template<class Stream>
      friend Stream& operator<<(Stream& os, const checker& checker)
      {
        os << checker.m_Logger;
        return os;
      }

      log_summary summary(std::string_view prefix) const
      {
        return log_summary{prefix, m_Logger};
      }
    protected:
      checker(checker&&)            = default;      
      ~checker()                    = default;
      
      checker& operator=(checker&&) = default;

      void log_critical_failure(std::string_view message) { m_Logger.log_critical_failure(message); }
      
      void log_failure(std::string_view message) { m_Logger.log_failure(message); }

      [[nodiscard]]
      std::size_t checks() const noexcept { return m_Logger.checks(); }

      [[nodiscard]]
      std::size_t failures() const noexcept { return m_Logger.failures(); }
            
      void post_message(std::string_view message) { m_Logger.post_message(message); }

      [[nodiscard]]
      const std::string& current_message() const noexcept{ return m_Logger.current_message(); }

      [[nodiscard]]
      int exceptions_detected_by_sentinel() const noexcept { return m_Logger.exceptions_detected_by_sentinel(); }

      typename Logger::sentinel make_sentinel(std::string_view message)
      {
        return {m_Logger, message};
      }
    private:
      Logger m_Logger;      
    };    
  }
}
