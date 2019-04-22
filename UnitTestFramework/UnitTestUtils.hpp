////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestUtils.hpp
    \brief Utilties for the unit testing framework.
*/

#include "StatisticalAlgorithms.hpp"
#include "TypeTraits.hpp"

#include <sstream>
#include <fstream>
#include <chrono>
#include <random>
#include <future>
#include <utility>
#include <set>
#include <array>

#ifndef _MSC_VER_
  #include <cxxabi.h>
#endif

namespace sequoia
{
  namespace unit_testing
  {
    template<class T, class=std::void_t<>> struct is_serializable : public std::false_type
    {};

    // Seems to be a bug here since decltype returns std::stringstream, whatever T!
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

    template<class Iter> void pad_right(Iter begin, Iter end, std::string_view suffix)
    {
       auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
            return lhs.size() < rhs.size();
        })
      };

      const auto maxChars{maxIter->size()};

      for(; begin != end; ++begin)
      {
        auto& s{*begin};
        s += std::string(maxChars - s.size(), ' ') += std::string{suffix};
      }
    }  
      
    template<class Iter> void pad_left(Iter begin, Iter end)
    {
       auto maxIter{std::max_element(begin, end, [](const std::string& lhs, const std::string& rhs) {
            return lhs.size() < rhs.size();
        })
      };

      const auto maxChars{maxIter->size()};

      for(; begin != end; ++begin)
      {
        auto& s{*begin};
        s = std::string(maxChars - s.size(), ' ') + s;
      }
    }
    
    enum class test_mode { standard, false_positive, false_negative };

    [[nodiscard]]
    inline constexpr bool diagnostic(const test_mode mode) noexcept { return (mode != test_mode::standard ); }
    
    template<test_mode Mode>
    class unit_test_logger
    {
    public:
      unit_test_logger()  = default;
      ~unit_test_logger() = default;
      
      unit_test_logger(const unit_test_logger&)            = delete;
      unit_test_logger(unit_test_logger&&)                 = default;
      unit_test_logger& operator=(const unit_test_logger&) = delete;
      unit_test_logger& operator=(unit_test_logger&&)      = delete;

      constexpr static test_mode mode{Mode};
      
      class [[nodiscard]] sentinel
      {
      public:        
        sentinel(unit_test_logger& logger, std::string_view message)
          : m_Logger{logger}
        {
          if(!logger.depth()) logger.current_message(message);
          if constexpr (diagnostic(Mode))
          {
            if(!logger.depth()) logger.log_check();
          }
          logger.increment_depth();
        }

        ~sentinel()
        {
          auto& logger = m_Logger.get();
          logger.decrement_depth();
          if constexpr (diagnostic(Mode))
          {
            if(!logger.depth())
            {
              const bool failure{((Mode == test_mode::false_positive) && !logger.failures()) || ((Mode == test_mode::false_negative) && logger.failures())};
              if(failure)
              {
                const std::string message{(Mode == test_mode::false_positive) ? logger.failure_description(logger.current_message()) : ""};
                logger.log_diagnostic_failure(message);
              }
              logger.reset_failures();
            }
          }

          if(!logger.depth()) logger.exceptions_detected_by_sentinel(std::uncaught_exceptions());
        }

        sentinel(const sentinel&) = delete;
        sentinel(sentinel&&)      = default;

        sentinel& operator=(const sentinel&) = delete;
        sentinel& operator=(sentinel&&)      = delete;
        
        void log_check()
        {
          if(!diagnostic(Mode)) m_Logger.get().log_check();
        }

        void log_performance_check()
        {
          if(!diagnostic(Mode)) m_Logger.get().log_performance_check();
        }
      private:
        std::reference_wrapper<unit_test_logger> m_Logger;
      };       
      
      void log_failure(std::string_view message)
      {
        ++m_Failures;
        if constexpr (Mode != test_mode::false_positive) m_Messages += (std::string{message} + '\n');
        if(std::ofstream of{m_RecoveryFile}) of << m_Messages;
      }

      void log_performance_failure(std::string_view message)
      {
        ++m_PerformanceFailures;
        log_failure(message);
      }

      void log_critical_failure(std::string_view message)
      {
        ++m_CriticalFailures;
        m_Messages.append(message) += '\n';
        if(std::ofstream of{m_RecoveryFile}) of << m_Messages;
      }

      void post_message(std::string_view message)
      {
        m_Messages.append(message);
      }

      [[nodiscard]]
      std::size_t failures() const noexcept { return m_Failures; }

      [[nodiscard]]
      std::size_t performance_failures() const noexcept { return m_PerformanceFailures; }     

      [[nodiscard]]
      std::size_t diagnostic_failures() const noexcept { return m_DiagnosticFailures; }

      [[nodiscard]]
      std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

      [[nodiscard]]
      std::size_t checks() const noexcept { return m_Checks; }

      [[nodiscard]]
      std::size_t performance_checks() const noexcept { return m_PerformanceChecks; } 

      [[nodiscard]]
      const std::string& failure_message_prefix() const noexcept { return m_FailureMessagePrefix; }
      
      void failure_message_prefix(std::string_view prefix) { m_FailureMessagePrefix = prefix; }

      [[nodiscard]]
      std::string failure_description(std::string_view description) const
      {
        const std::string prefix{failure_message_prefix()};
        std::string message{prefix.empty() ? failure_message_preface() : "\n\t[" + prefix + "] " + failure_message_preface()};
        
        if(!description.empty()) message.append(" (").append(description).append(")");

        return message;
      }

      [[nodiscard]]
      const std::string& messages() const noexcept{ return m_Messages; }

      [[nodiscard]]
      const std::string& current_message() const noexcept { return m_CurrentMessage; }

      void exceptions_detected_by_sentinel(const int n) { m_ExceptionsInFlight = n; }

      [[nodiscard]]
      int exceptions_detected_by_sentinel() const noexcept { return m_ExceptionsInFlight; }
    private:
      std::string
        m_Messages,
        m_FailureMessagePrefix,
        m_CurrentMessage;

      // TO DO: use std::filesystem once supported!
      std::string m_RecoveryFile{"../output/Recovery.txt"};

      int m_ExceptionsInFlight{};
      
      std::size_t
        m_Failures{},
        m_PerformanceFailures{},
        m_Checks{},
        m_PerformanceChecks{},
        m_Depth{},
        m_DiagnosticFailures{},
        m_CriticalFailures{};

      [[nodiscard]]
      std::size_t depth() const noexcept { return m_Depth; }

      void increment_depth() noexcept { ++m_Depth; }

      void decrement_depth() noexcept { --m_Depth; }

      void log_check() noexcept { ++m_Checks; }

      void log_performance_check() noexcept
      {
        log_check();
        ++m_PerformanceChecks;
      }

      void reset_failures() noexcept { m_Failures = 0u; }

      void log_diagnostic_failure(std::string_view message)
      {
        ++m_DiagnosticFailures;
        m_Messages.append(message) += '\n';
      }

      void current_message(std::string_view message)
      {
        m_CurrentMessage = message;
        if(std::ofstream of{m_RecoveryFile}) of << m_CurrentMessage;            
      }

      [[nodiscard]]
      static std::string prefix()
      {
        switch (Mode)
        {
          case test_mode::standard:
            return " ";
          case test_mode::false_positive:
            return " False positive diagnostic ";
          case test_mode::false_negative:
            return " False negative diagnostic ";
        }
       }

      [[nodiscard]]
      std::string failure_message_preface() const
      {
        return {"\n\tError -- check number " + std::to_string(checks()) +  ": "};
      }
    };

    enum class log_verbosity { terse=0, absent_checks=1, failure_messages=2};

    [[nodiscard]]
    constexpr log_verbosity operator&(log_verbosity lhs, log_verbosity rhs) noexcept
    {
      return static_cast<log_verbosity>(static_cast<int>(lhs) & static_cast<int>(rhs));
    }
    
    class log_summary
    {
    public:
      log_summary(std::string_view name="") : m_Name{name}
      {
      }
      
      template<test_mode Mode> log_summary(std::string_view name, const unit_test_logger<Mode>& logger)
        : m_Name{name}
        , m_Message{logger.messages()}
        , m_CurrentMessage{logger.current_message()}
      {
        switch(Mode)
        {
        case test_mode::standard:
          m_StandardFailures    = logger.failures() - logger.performance_failures();
          m_PerformanceFailures = logger.performance_failures();
          m_Checks              = logger.checks() - logger.performance_checks();
          m_PerformanceChecks   = logger.performance_checks();
          break;
        case test_mode::false_negative:
          m_FalseNegativeFailures = logger.diagnostic_failures();
          m_FalseNegativeChecks   = logger.checks();
          break;
        case test_mode::false_positive:
           m_FalsePositiveFailures = logger.diagnostic_failures();
           m_FalsePositiveChecks   = logger.checks();
          break;
        }

        m_CriticalFailures = logger.critical_failures();
      }

      void clear()
      {
        log_summary clean{""};
        std::swap(*this, clean);
      }

      [[nodiscard]]
      std::size_t checks() const noexcept { return m_Checks; }

      [[nodiscard]]
      std::size_t performance_checks() const noexcept { return m_PerformanceChecks; }

      [[nodiscard]]
      std::size_t false_negative_checks() const noexcept { return m_FalseNegativeChecks; }

      [[nodiscard]]
      std::size_t false_positive_checks() const noexcept { return m_FalsePositiveChecks; }

      [[nodiscard]]
      std::size_t standard_failures() const noexcept { return m_StandardFailures; }

      [[nodiscard]]
      std::size_t performance_failures() const noexcept { return m_PerformanceFailures; }

      [[nodiscard]]
      std::size_t false_negative_failures() const noexcept { return m_FalseNegativeFailures; }

      [[nodiscard]]
      std::size_t false_positive_failures() const noexcept { return m_FalsePositiveFailures; }

      [[nodiscard]]
      std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

      [[nodiscard]]
      std::string current_message() const
      {
        return m_Name.empty() ?  m_CurrentMessage : '[' + m_Name + "]\n\t" + m_CurrentMessage;
      }
      
      log_summary& operator+=(const log_summary& rhs)
      {
        m_Message               += rhs.m_Message;
        m_Checks                += rhs.m_Checks;
        m_PerformanceChecks     += rhs.m_PerformanceChecks;
        m_FalseNegativeChecks   += rhs.m_FalseNegativeChecks;
        m_FalsePositiveChecks   += rhs.m_FalsePositiveChecks;
        m_StandardFailures      += rhs.m_StandardFailures;
        m_PerformanceFailures   += rhs.m_PerformanceFailures;
        m_FalseNegativeFailures += rhs.m_FalseNegativeFailures;
        m_FalsePositiveFailures += rhs.m_FalsePositiveFailures;
        m_CriticalFailures      += rhs.m_CriticalFailures;
        m_ExceptionsInFlight    += rhs.m_ExceptionsInFlight;
        
        m_CurrentMessage         = rhs.m_CurrentMessage;

        return *this;
      }

      [[nodiscard]]
      const std::string& name() const noexcept { return m_Name; }
      
      void name(std::string_view p) { m_Name = p; }      
      
      [[nodiscard]]
      std::string summarize(std::string_view checkNumsPrefix, const log_verbosity suppression ) const
      {
        std::array<std::string, 4> summaries{
          std::string{checkNumsPrefix}.append("Standard Checks:"),
          std::string{checkNumsPrefix}.append("Performance Checks:"),
          std::string{checkNumsPrefix}.append("False Negative Checks:"),
          std::string{checkNumsPrefix}.append("False Positive Checks:")
        };

        pad_right(summaries.begin(), summaries.end(), "  ");

        std::array<std::string, 4> checkNums{
          std::to_string(checks()),
          std::to_string(performance_checks()),
          std::to_string(false_negative_checks()),
          std::to_string(false_positive_checks())
        };

        pad_left(checkNums.begin(), checkNums.end());

        const auto len{10u - std::min(std::size_t{8}, checkNums.front().size())};

        for(int i{}; i<4; ++i)
        {
          summaries[i].append(checkNums[i] + ";").append(len, ' ').append("Failures: ");
        }

        std::array<std::string, 4> failures{
          std::to_string(standard_failures()),
          std::to_string(performance_failures()),
          std::to_string(false_negative_failures()),
          std::to_string(false_positive_failures())
        };

        pad_left(failures.begin(), failures.end());

        for(int i{}; i<4; ++i)
        {
          summaries[i] += failures[i];
        }

        std::string summary{m_Name.empty() ? "" : m_Name + ":\n"};

        if((suppression & log_verbosity::absent_checks) == log_verbosity::absent_checks)
        {
          std::for_each(std::cbegin(summaries), std::cend(summaries), [&summary](const std::string& s){
              (summary += s) += "\n";
            }
          );
        }
        else
        {
          const std::array<std::size_t, 4> numbers{
            checks(),
            performance_checks(),
            false_negative_checks(),
            false_positive_checks()
           };

          for(int i{}; i<4; ++i)
          {
            
            if(numbers[i]) summary += summaries[i] += "\n";
          }
        }

        if(m_CriticalFailures)
        {
          (summary += "Critical Failures:  ") += std::to_string(critical_failures()) += "\n";
        }

        if((suppression & log_verbosity::failure_messages) == log_verbosity::failure_messages)
          summary += m_Message;

        return summary;
      }

      [[nodiscard]]
      friend log_summary operator+(const log_summary& lhs, const log_summary& rhs)
      {
        log_summary s{lhs};
        s += rhs;
        return s;
      }
    private:
      std::string m_Name, m_Message, m_CurrentMessage;
      std::size_t
           m_Checks{},
           m_PerformanceChecks{},
           m_FalseNegativeChecks{},
           m_FalsePositiveChecks{},
           m_StandardFailures{},
           m_PerformanceFailures{},
           m_FalseNegativeFailures{},
           m_FalsePositiveFailures{},
           m_CriticalFailures{};

      int m_ExceptionsInFlight{};

      [[nodiscard]]
      static std::string pluralize(const std::size_t n, std::string_view sv)
      {
        const std::string s{sv};
        return (n==1) ? " " + s : " " + s + "s";
      }
    };
    
    template<class T> struct equality_checker;

    template<class T> struct details_checker;

    template<class T> struct equivalence_checker;

    template<class T> struct weak_equivalence_checker;

    template<class T, class=std::void_t<>> struct has_details_checker : public std::false_type
    {};

    template<class T> struct has_details_checker<T, std::void_t<decltype(details_checker<T>{})>> : public std::true_type
    {};

    template<class T> constexpr bool has_details_checker_v{has_details_checker<T>::value};

    struct equality_tag{};
    struct equivalence_tag{};
    struct weak_equivalence_tag{};
    
    namespace impl
    {      
      inline std::string concat_messages(std::string_view s1, std::string_view s2)
      {
        std::string mess{!s1.empty() ? s1 : s2};
        if(!s1.empty() && !s2.empty())
        {
          mess.append(" ").append(s2);
        }
        
        return mess;
      }
      
      template<class T, class = void> struct container_detector : std::false_type
      {
      };

      template<class T> struct container_detector<T, std::void_t<decltype(std::declval<T>().cbegin())>> : std::true_type
      {
      };

      template<class T> constexpr bool container_detector_v{container_detector<T>::value};

      template<class Logger, std::size_t I = 0, class... T>
      std::enable_if_t<I == sizeof...(T), void>
      check_tuple_elements(Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, std::string_view description="")
      {}

      template<class Logger, std::size_t I = 0, class... T>
      std::enable_if_t<I < sizeof...(T), void>
      check_tuple_elements(Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, std::string_view description="")
      {
        using S = std::decay_t<decltype(std::get<I>(prediction))>;
        const std::string message{"Tuple elements differ for " + std::to_string(I) + "th element"};
        equality_checker<S>::check(logger, std::get<I>(value), std::get<I>(prediction), concat_messages(description, message));
        check_tuple_elements<Logger, I+1, T...>(logger, value, prediction, description);
      }

      template<class EquivChecker, class Logger, class T, class S, class... U>
      bool check(Logger& logger, const T& value, const S& s, const U&... u, std::string_view description)
      {      
        using sentinel = typename Logger::sentinel;
      
        sentinel r{logger, description};
        const auto previousFailures{logger.failures()};

        EquivChecker::check(logger, value, s, u..., description);

        constexpr bool falsePosMode{Logger::mode == test_mode::false_positive};
        const bool additionalFailures{logger.failures() != previousFailures};
      
        const bool passed{(falsePosMode && additionalFailures) || (!falsePosMode && !additionalFailures)};
        if(!passed)
        {
          std::string mess{description.empty() ? "Undescribed failure" : description};
          logger.post_message('\t' + mess + '\n');
        }
      
        return passed;
      }     
    }

    template<class T>
    struct equality_checker
    {       
      template<class Logger, class S=T>
      static std::enable_if_t<!impl::container_detector_v<S>, bool>
      check(Logger& logger, const T& value, const T& prediction, std::string_view description="")
      {        
        typename Logger::sentinel s{logger, description};
        s.log_check();
        const bool equal{prediction == value};
        if(!equal)
        {
          const std::string message{
            logger.failure_description(description) + "\n\t"
              + to_string(value) +  " differs from " + to_string(prediction)
              };
        
          logger.log_failure(message);
        }
        return equal;
      }

      template<class Logger, class S=T>
      static std::enable_if_t<impl::container_detector_v<S>, bool>
      check(Logger& logger, const T& value, const T& prediction, std::string_view description="")
      {
        typename Logger::sentinel r{logger, description};
        const bool equal{equality_checker<bool>::check(logger, true, prediction == value)};
        if(!equal)
        {
          if(equality_checker<std::size_t>::check(logger, value.size(), prediction.size(), std::string{description}.append("container size wrong")))
          {
            if constexpr (!diagnostic(Logger::mode))
            {
              const std::string message{"\nContainers both of size " + std::to_string(prediction.size()) +  "; checking elements"};
              logger.post_message(message);
            }
          
            for(auto predictionIter{std::begin(prediction)}, iter{std::begin(value)}; predictionIter != std::end(prediction); ++predictionIter, ++iter)
            {
              const std::string dist{std::to_string(std::distance(std::begin(prediction), predictionIter))};
              equality_checker<typename T::value_type>::check(logger, *iter, *predictionIter, std::string{description}.append("element ") += dist);
            }
          }
        }
        return equal;
      }      
    };

    template<class T, class S>
    struct equality_checker<std::pair<T,S>>
    {
      template<class Logger> static bool check(Logger& logger, const std::pair<T,S>& value, const std::pair<T,S>& prediction, std::string_view description="")
      {
        typename Logger::sentinel r{logger, description};
        r.log_check();
        const bool equal{prediction == value};
        if(!equal)
        {
          equality_checker<T>::check(logger, value.first, prediction.first, description);
          equality_checker<S>::check(logger, value.second, prediction.second, description);
        }
        return equal;
      }
    };

    template<class... T>
    struct equality_checker<std::tuple<T...>>
    {
      template<class Logger> static bool check(Logger& logger, const std::tuple<T...>& value, const std::tuple<T...>& prediction, std::string_view description="")
      {
        typename Logger::sentinel r{logger, description};
        const bool equal{equality_checker<bool>::check(logger, true, prediction == value, description)};
        if(!equal)
        {
          impl::check_tuple_elements(logger, value, prediction, description);
        }
        return equal;
      }
    };

    template<class Logger, class T>
    bool check(Logger& logger, equality_tag, const T& value, const T& prediction, std::string_view description)
    {
      static_assert(has_details_checker_v<T> || is_serializable_v<T>, "Provide either a specialization of details_checker or string_maker");
      
      using sentinel = typename Logger::sentinel;

      const auto priorFailures{logger.failures()};

      if constexpr(has_details_checker_v<T>)
      {
        if constexpr(is_equal_to_comparable_v<T>)
        {
          sentinel s{logger, description};
          s.log_check();
          if(!(prediction == value))
          {
            logger.log_failure(impl::concat_messages(description, "Equal-to comparison failed"));
          }
        }

        if constexpr(is_not_equal_to_comparable_v<T>)
        {
          sentinel s{logger, description};
          s.log_check();
          if(prediction != value)
          {
            logger.log_failure(impl::concat_messages(description, "Spurious not-equal-to comparison"));
          }
        }

        sentinel s{logger, description};
        details_checker<T>::check(logger, value, prediction, description);
      }
      else
      {
        sentinel s{logger, description};     
        equality_checker<T>::check(logger, value, prediction, description);
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
    
    template<class Logger, class T> bool check_equality(Logger& logger, const T& value, const T& prediction, std::string_view description="")
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

    template<class Logger> bool check(Logger& logger, const bool value, std::string_view description="")
    {
      return check_equality(logger, value, true, description);
    }

    template<class Logger, class T>
    bool check_equality_within_tolerance(Logger& logger, const T& value, const T& prediction, const T& tol, std::string_view description="")
    {
      typename Logger::sentinel r{logger, description};
      const bool equal{equality_checker<bool>::check(logger, (value > prediction - tol) && (value < prediction + tol), true)};
      if(!equal)
      {
        const std::string message{
          logger.failure_description(description) + "\n\t" + to_string(value) + " differs from " + to_string(prediction)
            + " by more than " + to_string(tol) + "\n"
            };

        logger.log_failure(message);
      }
      return equal;
    }

    template<class Logger, class E, class Fn>
    bool check_exception_thrown(Logger& logger, Fn&& function, std::string_view description="")
    {
      typename Logger::sentinel r{logger, description};
      r.log_check();
      try
      {
        function();
        const std::string message{logger.failure_description(description)
            + "\n\t" + "No exception thrown\n"};
        logger.log_failure(message);
        return false;
      }
      catch(E&)
      {
        return true;
      }
      catch(std::exception& e)
      {
        const std::string message{logger.failure_description(description)
            + "\n\t" + "unexpected std::exception thrown - \"" + e.what() + "\"\n"};
        logger.log_failure(message);
        return false;
      }
      catch(...)
      {
        const std::string message{logger.failure_description(description) + "\n\tError: Unknown exception thrown\n"};
        logger.log_failure(message);
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
        if(check_equality(logger, distance(first, last), predictedSize, impl::concat_messages(description, "container size wrong")))
        {
          auto predictionIter{predictionFirst};
          auto iter{first};
          for(; predictionIter != predictionLast; ++predictionIter, ++iter)
          {
            const std::string dist{std::to_string(std::distance(predictionFirst, predictionIter))};
            if(!check(logger, tag, *iter, *predictionIter, impl::concat_messages(description, "element ") += dist)) equal = false;
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
    void check_regular_semantics(Logger& logger, const T& x, const T& y, std::string_view description="")
    {
      typename Logger::sentinel s{logger, description};

      if(!check(logger, x == x, impl::concat_messages(description, "Equality operator is inconsist"))) return;
      if(!check(logger, !(x != x), impl::concat_messages(description, "Inequality operator is inconsistent"))) return;

      // TO DO: contract in C++20
      if(!check(logger, x != y, impl::concat_messages(description, "Precondition - for check the standard semantics, x and y are assumed to be different"))) return;
      
      auto z{x};
      check_equality(logger, x, z, impl::concat_messages(description, "Copy constructor"));
      check(logger, z == x, impl::concat_messages(description, "Equality operator"));

      z = y;
      check_equality(logger, y, z, impl::concat_messages(description, "Copy assignment"));
      check(logger, z != x, impl::concat_messages(description, "Inequality operator"));

      auto w{std::move(z)};
      check_equality(logger, y, w, impl::concat_messages(description, "Move constructor"));

      z = [x](){ return x;}();
      check_equality(logger, x, z, impl::concat_messages(description, "Move assignment"));      

      std::swap(w,z);
      check_equality(logger, x, w, impl::concat_messages(description, "Swap"));
      check_equality(logger, y, z, impl::concat_messages(description, "Swap"));
    }
    
    template<class R> struct performance_results
    {
      std::vector<std::future<R>> fast_futures, slow_futures;
      bool passed{};
    };

    template<class Logger, class F, class S>
    auto check_relative_performance(Logger& logger, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials, const double num_sds, const bool reportSuccess, std::string_view description) -> performance_results<std::invoke_result_t<F>>
    {      
      using R = std::invoke_result_t<F>;
      static_assert(std::is_same_v<R, std::invoke_result_t<S>>, "Fast/Slow invokables must have same return value");
      
      // Replace with contracts in 2020
      if((minSpeedUp <= 1) || (maxSpeedUp <= 1))
        throw std::logic_error("Relative performance test requires speed-up factors > 1!");

      if(minSpeedUp > maxSpeedUp)
        throw std::logic_error("maxSpeedUp must be >= minSpeedUp");      

      typename Logger::sentinel r{logger, description};
      r.log_performance_check();
      
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
          message << "\n\tFast Task duration: " << m_f << "s";
          message << " +- " << num_sds << " * " << sig_f;
          message << "\n\tSlow Task duration: " << m_s << "s";
          message << " +- " << num_sds << " * " << sig_s;
          message << " [" << m_s / m_f << "; (" << minSpeedUp << ", " << maxSpeedUp << ")]";

          return message.str();
        }
      };

      if(!results.passed)
      {
        const std::string message{logger.failure_description(description) + serializer()};
        logger.log_performance_failure(message);
      }
      else if(reportSuccess)
      {
        logger.post_message(serializer());
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

      template<class T> bool check_equality(const T& value, const T& prediction, std::string_view description="")
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
      bool check_equality_within_tolerance(const T prediction, const T value, const T tol, std::string_view description="")
      {
        return unit_testing::check_equality_within_tolerance(m_Logger, value, prediction, tol, description);
      }

      bool check(const bool value, std::string_view description="")
      {
        return unit_testing::check(m_Logger, value, description);
      }

      template<class E, class Fn>
      bool check_exception_thrown(Fn&& function, std::string_view description="")
      {
        return unit_testing::check_exception_thrown<Logger, E, Fn>(m_Logger, std::forward<Fn>(function), description);
      }

      template<class Iter, class PredictionIter>
      bool check_range(Iter first, Iter last, PredictionIter predictionFirst, PredictionIter predictionLast, std::string_view description="")
      {
        return unit_testing::check_range(m_Logger, first, last, predictionFirst, predictionLast, description);
      }

      template<class T>
      void check_regular_semantics(const T& x, const T& y, std::string_view description="")
      {
        unit_testing::check_regular_semantics(m_Logger, x, y, description);        
      }
      
      template<class F, class S>
      auto check_relative_performance(F fast, S slow, const double minSpeedUp, const double maxSpeedUp, std::string_view description="", const std::size_t trials=5, const double num_sds=3)
        -> performance_results<decltype(fast())>
      {
        constexpr bool verboseIfPassed{Logger::mode == test_mode::false_positive};
        return unit_testing::check_relative_performance(m_Logger, fast, slow, minSpeedUp, maxSpeedUp, trials, num_sds, verboseIfPassed, description);
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
      
      void failure_message_prefix(std::string_view prefix) { m_Logger.failure_message_prefix(prefix); }

      [[nodiscard]]
      const std::string& failure_message_prefix() const noexcept{ return m_Logger.failure_message_prefix(); }
      
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

    class test
    {
    public:
      test(std::string_view name) : m_Name{name} {}
      virtual ~test() = default;

      virtual log_summary execute() = 0;

      const std::string& name() const noexcept { return m_Name; }
    protected:
      test(const test&) = default;
      test(test&&)      = default;

      test& operator=(const test&) = default;
      test& operator=(test&&)      = default;
      
    private:
      std::string m_Name;
    };
    
    template<class Logger, class Checker=checker<Logger>>
    class basic_test : public test, protected Checker
    {
    public:      
      basic_test(std::string_view name) : test{name} {}
      
      log_summary execute() override
      {        
        try
        {
          run_tests();
        }
        catch(std::exception& e)
        {         
          Checker::log_critical_failure(make_message("Unexpected", e.what()));
        }
        catch(...)
        {
          Checker::log_critical_failure(make_message("Unknown", ""));
        }

        return Checker::summary(name());
      }
    protected:
      virtual void run_tests() = 0;

      virtual std::string current_message() const { return std::string{Checker::failure_message_prefix()} + std::string{Checker::current_message()}; }

      std::string make_message(std::string_view tag, std::string_view exceptionMessage) const
      {
        auto mess{(std::string{"\tError -- "}.append(tag) += " Exception:") += format(exceptionMessage) += '\n'};
        if(Checker::exceptions_detected_by_sentinel())
        {
          mess += "\tException thrown during last check\n";
        }
        else
        {
          mess += "\tException thrown after check completed\n";
        }

        mess += ("\tLast Recorded Message:\n" + format(current_message()));

        return mess;
      }

      static std::string format(std::string_view s)
      {
        return s.empty() ? std::string{s} : std::string{'\t'}.append(s);
      }
    };

    using unit_test = basic_test<unit_test_logger<test_mode::standard>>;
    using false_positive_test = basic_test<unit_test_logger<test_mode::false_positive>>;
    using false_negative_test = basic_test<unit_test_logger<test_mode::false_negative>>;    

    class test_family
    {
    public:
      template<class... Tests>
      test_family(std::string_view name, Tests&&... tests) : m_Name{name}
      {
        register_tests(std::forward<Tests>(tests)...);
      }

      std::vector<log_summary> execute()
      {
        std::vector<log_summary> summaries{};
        for(auto& pTest : m_Tests)
          {
            summaries.push_back(pTest->execute());
          }

        return summaries;
      }

      [[nodiscard]]
      std::string_view name() const noexcept { return m_Name; }      
    private:
      std::vector<std::unique_ptr<test>> m_Tests{};
      std::string m_Name;

      template<class Test, class... Tests>
      void register_tests(Test&& t, Tests&&... tests)
      {
        m_Tests.emplace_back(std::make_unique<Test>(std::forward<Test>(t)));
        register_tests(std::forward<Tests>(tests)...);
      }

      void register_tests() {}
    };
    
    inline std::string report_line(std::string file, const int line, const std::string_view message)
    {
      std::string s{std::move(file) + ", Line " + std::to_string(line)};
      if(!message.empty()) (s += ": ") += message;

      return s;
    }

    #define LINE(message) report_line(__FILE__, __LINE__, message)

    template<class T> std::string demangle()
    {
      #ifndef _MSC_VER_
        int status;
        return abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
      #else        
        return typeid(T).name();
      #endif
    }
  }
}
