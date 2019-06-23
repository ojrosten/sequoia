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
    
    namespace impl
    {
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
            
      template<class EquivChecker, class Logger, class T, class S, class... U>
      bool check(Logger& logger, const T& value, const S& s, const U&... u, std::string_view description)
      {
        using sentinel = typename Logger::sentinel;

        const std::string message{
          add_type_info<S, U...>(
            combine_messages(description, "Comparison performed using:\n\t[" + demangle<EquivChecker>() + "]\n\tWith equivalent types:", "\n"))
        };
      
        sentinel r{logger, message};
        const auto previousFailures{logger.failures()};

        EquivChecker::check(logger, value, s, u..., message);
      
        return logger.failures() == previousFailures;
      }

      template<class Logger, class T, class... Args>
      void check_regular_semantics(Logger& logger, const T& x, const T& y, std::string_view description, const Args&... args)
      {
        typename Logger::sentinel s{logger, impl::add_type_info<T>(description)};

        if(!check(logger, x == x, combine_messages(description, "Equality operator is inconsistent"))) return;
        if(!check(logger, !(x != x), combine_messages(description, "Inequality operator is inconsistent"))) return;

        // TO DO: contract in C++20
        if(!check(logger, x != y, combine_messages(description, "Precondition - for check the standard semantics, x and y are assumed to be different"))) return;
      
        T z{x, args...};
        check_equality(logger, z, x, combine_messages(description, "Copy constructor"));
        check(logger, z == x, combine_messages(description, "Equality operator"));

        z = y;
        check_equality(logger, z, y, combine_messages(description, "Copy assignment"));
        check(logger, z != x, combine_messages(description, "Inequality operator"));

        T w{std::move(z), args...};
        check_equality(logger, w, y, combine_messages(description, "Move constructor"));

        z = [&x]() -> T { return x; }();
        check_equality(logger, z, x, combine_messages(description, "Move assignment"));      

        std::swap(w,z);
        check_equality(logger, w, x, combine_messages(description, "Swap"));
        check_equality(logger, z, y, combine_messages(description, "Swap"));
      }
    }

    template<class Logger, class Iter, class PredictionIter>
    bool check_range(Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description);

    template<class Logger, class T>
    bool check(Logger& logger, equality_tag, const T& value, const T& prediction, std::string_view description)
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
        sentinel s{logger, impl::add_type_info<T>(description)};
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
          sentinel s{logger, impl::add_type_info<T>(description)};
          s.log_check();
          if(prediction != value)
          {
            logger.log_failure(messageGenerator("!=", "true"));
          }
        }

        if constexpr(has_detailed_equality_checker_v<T>)
        {          
          sentinel s{logger, impl::add_type_info<T>(description)};
          detailed_equality_checker<T>::check(logger, value, prediction, description);
        }
        else if constexpr(is_container_v<T>)
        {
          check_range(logger, std::begin(value), std::end(value), std::begin(prediction), std::end(prediction), description);
        }      
        else
        {
          static_assert(dependent_false<T>::value, "Unable to check the details for input type");
        }
      }

      return logger.failures() == priorFailures;
    }
    
    template<class Logger, class T, class S, class... U>
    bool check(Logger& logger, equivalence_tag, const T& value, S&& s, U&&... u, std::string_view description)
    {
      return impl::check<equivalence_checker<T>, Logger, T, S, U...>(logger, value, std::forward<S>(s), std::forward<U>(u)..., description);      
    }

    template<class Logger, class T, class S, class... U>
    bool check(Logger& logger, weak_equivalence_tag, const T& value, S&& s, U&&... u, std::string_view description)
    {
      return impl::check<weak_equivalence_checker<T>, Logger, T, S, U...>(logger, value, std::forward<S>(s), std::forward<U>(u)..., description);
    }
    
    template<class Logger, class T> bool check_equality(Logger& logger, const T& value, const T& prediction, std::string_view description)
    {
      return check(logger, equality_tag{}, value, prediction, description);
    }

    template<class Logger, class T, class S, class... U>
    bool check_equivalence(Logger& logger, const T& value, S&& s, U&&... u, std::string_view description)
    {
      return check<Logger, T, S, U...>(logger, equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)..., description);      
    }

    template<class Logger, class T, class S, class... U>
    bool check_weak_equivalence(Logger& logger, const T& value, S&& s, U&&... u, std::string_view description)
    {
      return check<Logger, T, S, U...>(logger, weak_equivalence_tag{}, value, std::forward<S>(s), std::forward<U>(u)..., description);      
    }

    template<class Logger> bool check(Logger& logger, const bool value, std::string_view description)
    {
      return check_equality(logger, value, true, description);
    }

    template<class Logger, class T>
    bool check_equality_within_tolerance(Logger& logger, const T& value, const T& prediction, const T& tol, std::string_view description)
    {
      auto message{combine_messages(description, "Check using tolerance of " + std::to_string(tol))};
      return check(logger, (value > prediction - tol) && (value < prediction + tol), std::move(message));
    }

    template<class Logger, class E, class Fn>
    bool check_exception_thrown(Logger& logger, Fn&& function, std::string_view description)
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
      bool check_range(Logger& logger, Tag tag, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description)
      {
        typename Logger::sentinel r{logger, description};
        bool equal{true};

        using std::distance;
        const auto predictedSize{distance(predictionFirst, predictionLast)};
        if(check_equality(logger, distance(first, last), predictedSize, combine_messages(description, "container size wrong")))
        {
          auto predictionIter{predictionFirst};
          auto iter{first};
          for(; predictionIter != predictionLast; ++predictionIter, ++iter)
          {
            std::string dist{std::to_string(std::distance(predictionFirst, predictionIter)).append("\n")};
            if(!check(logger, tag, *iter, *predictionIter, combine_messages(description, "element ").append(std::move(dist)))) equal = false;
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
    bool check_range(Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description)
    {
      return impl::check_range(logger, equality_tag{}, first, last, predictionFirst, predictionLast, description);      
    }

    template<class Logger, class Iter, class PredictionIter>
    bool check_range_equivalence(Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description)
    {
      return impl::check_range(logger, equivalence_tag{}, first, last, predictionFirst, predictionLast, description);      
    }

    template<class Logger, class Iter, class PredictionIter>
    bool check_range_weak_equivalence(Logger& logger, Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description)
    {
      return impl::check_range(logger, weak_equivalence_tag{}, first, last, predictionFirst, predictionLast, description);      
    }

    
    template<class Logger, class T>
    void check_regular_semantics(Logger& logger, const T& x, const T& y, std::string_view description)
    {
      impl::check_regular_semantics(logger, x, y, description);
    }

    template<class Logger, class T, class Allocator>
    void check_regular_semantics(Logger& logger, const T& x, const T& y, const Allocator& allocator, std::string_view description)
    {
      impl::check_regular_semantics(logger, x, y, description, allocator);
    }

    template<class Logger, class T, class Mutator>
    void check_copy_consistency(Logger& logger, T& target, const T& prediction, Mutator m, std::string_view description)
    {
      typename Logger::sentinel s{logger, impl::add_type_info<T>(description)};

      auto c{target};
      
      m(target);
      check_equality(logger, target, prediction, combine_messages(description, "Mutation of target"));

      m(c);
      check_equality(logger, c, prediction, combine_messages(description, "Mutation of copy"));
      
    }

    template<class T, class S>
    struct detailed_equality_checker<std::pair<T, S>>
    {
      template<class Logger>
      static void check(Logger& logger, const std::pair<T, S>& value, const std::pair<T,S>& prediction, std::string_view description)
      {
        check_equality(logger, value.first, prediction.first, combine_messages(description, "First element of pair is incorrent"));
        check_equality(logger, value.second, prediction.second, combine_messages(description, "First element of pair is incorrect"));
      }
    };

    template<class... T>
    struct detailed_equality_checker<std::tuple<T...>>
    {     
    private:
      template<class Logger, std::size_t I = 0>
      static void check_tuple_elements(Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, std::string_view description)
      {
        if constexpr(I < sizeof...(T))
        {
          const std::string message{std::to_string(I) + "th element of tuple incorrect"};
          check_equality(logger, std::get<I>(value), std::get<I>(prediction), combine_messages(description, message));
          check_tuple_elements<Logger, I+1>(logger, value, prediction, description);
        }
      }
        
    public:
      template<class Logger>
      static void check(Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, std::string_view description)
      {
        check_tuple_elements(logger, value, prediction, description);
      }
    };
    
    template<class R> struct performance_results
    {
      std::vector<std::future<R>> fast_futures, slow_futures;
      bool passed{};
    };

    template<class Logger, class F, class S>
    auto check_relative_performance(Logger& logger, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials, const double num_sds, std::string_view description) -> performance_results<std::invoke_result_t<F>>
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

      template<class T> bool check_equality(const T& value, const T& prediction, std::string_view description)
      {
        return unit_testing::check_equality(m_Logger, value, prediction, description);
      }

      template<class T, class S, class... U>
      bool check_equivalence(const T& value, S&& s, U&&... u, std::string_view description)
      {
        return unit_testing::check_equivalence<Logger, T, S, U...>(m_Logger, value, std::forward<S>(s), std::forward<U>(u)..., description);
      }

      template<class T, class S, class... U>
      bool check_weak_equivalence(const T& value, S&& s, U&&... u, std::string_view description)
      {
        return unit_testing::check_weak_equivalence<Logger, T, S, U...>(m_Logger, value, std::forward<S>(s), std::forward<U>(u)..., description);
      }
      
      template<class T>
      bool check_equality_within_tolerance(const T prediction, const T value, const T tol, std::string_view description)
      {
        return unit_testing::check_equality_within_tolerance(m_Logger, value, prediction, tol, description);
      }

      bool check(const bool value, std::string_view description)
      {
        return unit_testing::check(m_Logger, value, description);
      }

      template<class E, class Fn>
      bool check_exception_thrown(Fn&& function, std::string_view description)
      {
        return unit_testing::check_exception_thrown<Logger, E, Fn>(m_Logger, std::forward<Fn>(function), description);
      }

      template<class Iter, class PredictionIter>
      bool check_range(Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description)
      {
        return unit_testing::check_range(m_Logger, first, last, predictionFirst, predictionLast, description);
      }

      template<class T>
      void check_regular_semantics(const T& x, const T& y, std::string_view description)
      {
        unit_testing::check_regular_semantics(m_Logger, x, y, description);        
      }

      template<class T, class Allocator>
      void check_regular_semantics(const T& x, const T& y, const Allocator& allocator, std::string_view description)
      {
        unit_testing::check_regular_semantics(m_Logger, x, y, allocator, description);        
      }

      template<class T, class Mutator>
      void check_copy_consistency(T& target, const T& prediction, Mutator m, std::string_view description)
      {
        unit_testing::check_copy_consistency(m_Logger, target, prediction, std::move(m), description);
      }
      
      template<class F, class S>
      auto check_relative_performance(F fast, S slow, const double minSpeedUp, const double maxSpeedUp, std::string_view description, const std::size_t trials=5, const double num_sds=3)
        -> performance_results<decltype(fast())>
      {
        return unit_testing::check_relative_performance(m_Logger, fast, slow, minSpeedUp, maxSpeedUp, trials, num_sds, description);
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
