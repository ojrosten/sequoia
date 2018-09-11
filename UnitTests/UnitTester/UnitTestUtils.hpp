#pragma once

#include "Sample.hpp"
#include "Utilities.hpp"

#include <ostream>
#include <sstream>
#include <iostream>
#include <chrono>
#include <random>
#include <future>
#include <utility>
#include <set>

namespace sequoia
{
  namespace unit_testing
  {
    template<class T> std::string to_string(const T& value)
    {
      std::ostringstream os;
      os << value;
      return os.str();
    }
    
    enum class test_mode { standard, false_positive, false_negative };

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
      
      class sentinel
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

          if(!logger.depth()) logger.exceptions_detected_by_sentinel(std::uncaught_exception());
        }

        sentinel(const sentinel&) = delete;
        sentinel(sentinel&&)      = default;

        sentinel& operator=(const sentinel&) = delete;
        sentinel& operator=(sentinel&&)      = delete;
        
        void log_check()
        {
          if(!diagnostic(Mode)) m_Logger.get().log_check();
        }
      private:
        std::reference_wrapper<unit_test_logger> m_Logger;
      };       
      
      void log_failure(std::string message)
      {
        ++m_Failures;
        if constexpr (Mode != test_mode::false_positive) m_Messages += (std::move(message) + '\n');
      }

      void log_critical_failure(std::string message)
      {
        ++m_CriticalFailures;
        m_Messages += (std::move(message) + '\n');
      }

      void post_message(std::string message)
      {
        m_Messages += std::move(message);
      }

      std::size_t failures() const noexcept { return m_Failures; }      
      std::size_t checks() const noexcept { return m_Checks; }      
      std::size_t diagnostic_failures() const noexcept { return m_DiagnosticFailures; }
      std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

      std::string failure_message_prefix() const { return m_FailureMessagePrefix; }
      void failure_message_prefix(std::string_view prefix) { m_FailureMessagePrefix = prefix; }

      std::string failure_description(std::string_view description) const
      {
        const std::string prefix{failure_message_prefix()};
        std::string message{prefix.empty() ? failure_message_preface() : "\n\t[" + prefix + "] " + failure_message_preface()};
        
        if(!description.empty()) message += (" (" + std::string{description} + ")");

        return message;
      }

      std::string messages() const { return m_Messages; }

      std::string current_message() const { return m_CurrentMessage; }

      void exceptions_detected_by_sentinel(const int n) { m_ExceptionsInFlight = n; }
      int exceptions_detected_by_sentinel() const noexcept { return m_ExceptionsInFlight; }
    private:
      std::string
        m_Messages,
        m_FailureMessagePrefix,
        m_CurrentMessage;

      int m_ExceptionsInFlight{};
      
      std::size_t m_Failures{}, m_Checks{}, m_Depth{}, m_DiagnosticFailures{}, m_CriticalFailures{};
      
      std::size_t depth() const noexcept { return m_Depth; }
      void increment_depth() noexcept { ++m_Depth; }
      void decrement_depth() noexcept { --m_Depth; }

      void log_check() noexcept { ++m_Checks; }

      void reset_failures() noexcept { m_Failures = 0u; }

      void log_diagnostic_failure(std::string message)
      {
        ++m_DiagnosticFailures;
        m_Messages += (std::move(message) + '\n');
      }

      void current_message(std::string_view message)
      {
        m_CurrentMessage = message;
      }

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

      std::string failure_message_preface() const
      {
        return {"\n\tError -- check number " + std::to_string(checks()) +  ": "};
      }
    };

    class log_summary
    {
    public:
      log_summary(std::string_view prefix="") : m_Prefix{prefix}
      {
      }
      
      template<test_mode Mode> log_summary(std::string_view prefix, const unit_test_logger<Mode>& logger)
        : m_Prefix{prefix}
        , m_Message{logger.messages()}
        , m_CurrentMessage{logger.current_message()}
      {
        switch(Mode)
        {
        case test_mode::standard:
          m_StandardFailures = logger.failures();          
          m_Checks = logger.checks();
          break;
        case test_mode::false_negative:
          m_FalseNegativeFailures = logger.diagnostic_failures();
          m_FalseNegativeChecks = logger.checks();
          break;
        case test_mode::false_positive:
           m_FalsePositiveFailures = logger.diagnostic_failures();
           m_FalsePositiveChecks = logger.checks();
          break;
        }

        m_CriticalFailures = logger.critical_failures();
      }

      void clear()
      {
        log_summary clean{""};
        std::swap(*this, clean);
      }
      
      std::size_t checks() const noexcept { return m_Checks; }
      std::size_t false_negative_checks() const noexcept { return m_FalseNegativeChecks; }
      std::size_t false_positive_checks() const noexcept { return m_FalsePositiveChecks; }
      
      std::size_t standard_failures() const noexcept { return m_StandardFailures; }
      std::size_t false_negative_failures() const noexcept { return m_FalseNegativeFailures; }
      std::size_t false_positive_failures() const noexcept { return m_FalsePositiveFailures; }
      std::size_t critical_failures() const noexcept { return m_CriticalFailures; }

      int exceptions_detected_by_sentinel() const noexcept { return m_ExceptionsInFlight ; }
      
      std::string message() const noexcept
      {
        return m_Prefix + summary() + m_Message;
      }

      std::string current_message() const
      {
        return m_Prefix.empty() ?  m_CurrentMessage : '[' + m_Prefix + "]\n\t" + m_CurrentMessage;
      }

      log_summary& operator+=(const log_summary& rhs)
      {
        m_Message               += rhs.m_Message;
        m_Checks                += rhs.checks();
        m_FalseNegativeChecks   += rhs.false_negative_checks();
        m_FalsePositiveChecks   += rhs.false_positive_checks();
        m_StandardFailures      += rhs.standard_failures();
        m_FalseNegativeFailures += rhs.false_negative_failures();
        m_FalsePositiveFailures += rhs.false_positive_failures();
        m_CriticalFailures      += rhs.critical_failures();
        m_ExceptionsInFlight    += rhs.exceptions_detected_by_sentinel();

        m_CurrentMessage         = rhs.m_CurrentMessage;

        if(m_Prefix.empty()) m_Prefix = rhs.prefix();

        return *this;
      }

      std::string prefix() const { return m_Prefix; }
      void prefix(std::string_view p) { m_Prefix = p; }
      
      friend log_summary operator+(const log_summary& lhs, const log_summary& rhs)
      {
        log_summary s{lhs};
        s += rhs;
        return s;
      }
    private:
      std::string m_Prefix, m_Message, m_CurrentMessage;
      std::size_t m_Checks{},        
           m_FalseNegativeChecks{},
           m_FalsePositiveChecks{},
           m_StandardFailures{},
           m_FalseNegativeFailures{},
           m_FalsePositiveFailures{},
           m_CriticalFailures{};

      int m_ExceptionsInFlight{};

      std::string summary() const
      {
        std::string mess{};
        mess += summarize(checks(), standard_failures(), "");
        mess += summarize(false_negative_checks(), false_negative_failures(), "False negative ");
        mess += summarize(false_positive_checks(), false_positive_failures(), "False positive ");
        if(m_CriticalFailures)
        {
          using namespace std::string_literals;
          mess += "\n\t"s += std::to_string(m_CriticalFailures) += pluralize(m_CriticalFailures, "Critical Failure") += "\n"s;
        }
        return mess;
      }

      static std::string summarize(const std::size_t checks, const std::size_t failures, const std::string& prefix)
      {
        std::string m{};
        if(checks)
        {
          m = ("\t" + std::to_string(checks) + pluralize(checks, prefix + "Check") + " done\n");
          m += ("\t" + std::to_string(failures) + pluralize(failures, prefix + "Failure") + "\n");
        }
        return m;
      }

      static std::string pluralize(const std::size_t n, const std::string& s)
      {
        return (n==1) ? " " + s : " " + s + "s";
      }
    };

    namespace impl
    {
      template<class T, class = void> struct container_detector : std::false_type
      {
      };

      template<class T> struct container_detector<T, std::void_t<decltype(std::declval<T>().cbegin())>> : std::true_type
      {
      };
      
      template<class T, bool=container_detector<T>::value> struct equality_checker;
      
      template<class Logger, class T> bool check_equality(Logger& logger, const T reference, const T actual, const std::string& description="")
      {      
        typename Logger::sentinel r{logger, description};
        r.log_check();
        const bool equal{reference == actual};
        if(!equal)
        {
          const std::string message{
            logger.failure_description(description) + "\n\t"
              + to_string(actual) +  " differs from " + to_string(reference)
              };
        
          logger.log_failure(message);
        }
        return equal;
      }

      template<class Logger> bool check(Logger& logger, const bool value, const std::string& description="")
      {
        return impl::check_equality(logger, true, value, description);
      }
      
      template<class Logger, class T, class S>
      bool check_equality(Logger& logger, const std::pair<T,S>& reference, const std::pair<T,S>& actual, const std::string& description="")
      {
        typename Logger::sentinel r{logger, description};
        r.log_check();
        const bool equal{reference == actual};
        if(!equal)
        {
          equality_checker<T>::check(logger, reference.first, actual.first, description);
          equality_checker<S>::check(logger, reference.second, actual.second, description);
        }
        return equal;
      }

      template<class Logger, std::size_t I = 0, class... T>
      std::enable_if_t<I == sizeof...(T), void>
      check_tuple_elements(Logger& logger, const std::tuple<T...>& reference, const std::tuple<T...>& actual, const std::string& description="")
      {}

      template<class Logger, std::size_t I = 0, class... T>
      std::enable_if_t<I < sizeof...(T), void>
      check_tuple_elements(Logger& logger, const std::tuple<T...>& reference, const std::tuple<T...>& actual, const std::string& description="")
      {
        using S = std::decay_t<decltype(std::get<I>(reference))>;
        equality_checker<S>::check(logger, std::get<I>(reference), std::get<I>(actual), description);
        check_tuple_elements<Logger, I+1, T...>(logger, reference, actual, description);
      }

      template<class Logger, class... T>
      bool check_equality(Logger& logger, const std::tuple<T...>& reference, const std::tuple<T...>& actual, const std::string& description="")
      {
        typename Logger::sentinel r{logger, description};
        const bool equal{impl::check(logger, reference == actual)};
        if(!equal)
        {
          check_tuple_elements(logger, reference, actual, description);
        }
        return equal;
      }
      
      template<class Logger, class T>
      bool check_container(Logger& logger, const T& reference, const T& actual, const std::string& description="")
      {
        typename Logger::sentinel r{logger, description};
        const bool equal{impl::check(logger, reference == actual)};
        if(!equal)
        {
          if(impl::check_equality(logger, reference.size(), actual.size(), description + "container size wrong"))
          {
            if constexpr (!diagnostic(Logger::mode))
            {
              const std::string message{"\nContainers both of size " + std::to_string(reference.size()) +  "; checking elements"};
              logger.post_message(message);
            }
          
            for(auto refIter = std::begin(reference), actIter = std::begin(actual); refIter != std::end(reference); ++refIter, ++actIter)
            {
              const std::string dist{std::to_string(std::distance(std::begin(reference), refIter))};
              equality_checker<typename T::value_type>::check(logger, *refIter, *actIter, description +  "element " + dist);
            }
          }
        }
        return equal;
      }

      template<class T> struct equality_checker<T, false>
      {
        template<class Logger> static bool check(Logger& logger, const T reference, const T value, const std::string& description="")
        {
          return impl::check_equality(logger, reference, value, description);
        }
      };

      template<class T> struct equality_checker<T, true>
      {
        template<class Logger> static bool check(Logger& logger, const T reference, const T value, const std::string& description="")
        {
          return impl::check_container(logger, reference, value, description);
        }
      };
       
    }

    template<class Logger, class T> bool check_equality(Logger& logger, const T reference, const T value, const std::string& description="")
    {
      return impl::equality_checker<T>::check(logger, reference, value, description);
    }

    template<class Logger, class T>
    bool check_equality_within_tolerance(Logger& logger, const T reference, const T value, const T tol, const std::string& description="")
    {
      typename Logger::sentinel r{logger, description};
      const bool equal{impl::check(logger, (value > reference - tol) && (value < reference + tol))};
      if(!equal)
      {
        const std::string message{
          logger.failure_description(description) + "\n\t" + to_string(value) + " differs from " + to_string(reference)
            + " by more than " + to_string(tol) + "\n"
            };

        logger.log_failure(message);
      }
      return equal;
    }

    template<class Logger, class E, class Fn>
    bool check_exception_thrown(Logger& logger, Fn&& function, const std::string& description="")
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

    template<class Logger, class RefIter, class ResultIter>
    bool check_range(Logger& logger, RefIter refBegin, RefIter refEnd, ResultIter resBegin, ResultIter resEnd, const std::string& description="")
    {
      typename Logger::sentinel r{logger, description};
      bool equal{true};
      const auto refSize = std::distance(refBegin, refEnd);
      if(check_equality(logger, refSize, std::distance(resBegin, resEnd), description + " container size wrong"))
      {        
        for(auto refIter = refBegin, resIter = resBegin; refIter != refEnd; ++refIter, ++resIter)
        {
          const std::string dist{std::to_string(std::distance(refBegin, refIter))};
          if(!check_equality(logger, *refIter, *resIter, description +  "element " + dist)) equal = false;
        }
      }
      else
      {
        equal = false;
      }
      
      return equal;
    }
    
    template<class R> struct performance_results
    {
      std::vector<std::future<R>> fast_futures, slow_futures;
      bool passed{};
    };

    template<class Logger, class F, class S>
    auto check_relative_performance(Logger& logger, F fast, S slow, const double factor, const bool oneSided, const std::size_t trials, const double num_sds, const bool reportSuccess, const std::string& description) -> performance_results<decltype(fast())>
    {
      using R = decltype(fast());
      static_assert(std::is_same<R, decltype(slow())>::value, "Fast/Slow invokables must have same return value");

      typename Logger::sentinel r{logger, description};
      r.log_check();
      
      performance_results<R> results;

      if(factor < 0.0)
        throw std::runtime_error("Relative performance test requires a positive definite comparison factor!");

      using namespace std::chrono;
      using namespace statistics;

      sample<double> fastData, slowData;
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
        fastFirst ? fastData.add_datum(duration.count()) : slowData.add_datum(duration.count());

        start = steady_clock::now();
        fastFirst ? slowTask() : fastTask();
        end = steady_clock::now();

        duration = end - start;
        fastFirst ? slowData.add_datum(duration.count()) : fastData.add_datum(duration.count());
      }

      using namespace statistics::bias;
      if((factor == 1.0f) || (fastData.mean() + fastData.template sample_standard_deviation<gaussian_approx_modifier>()
                               < slowData.mean() - slowData.template sample_standard_deviation<gaussian_approx_modifier>()))
      {
        const auto fastSD = fastData.template sample_standard_deviation<gaussian_approx_modifier>();
        const auto slowSD = slowData.template sample_standard_deviation<gaussian_approx_modifier>();
        if(fastSD >= slowSD)
        {
          results.passed = factor*fastData.mean() < (slowData.mean() + num_sds*slowSD)
            && (oneSided || (factor*fastData.mean() > (slowData.mean() - num_sds*slowSD)));
        }
        else
        {
          results.passed = ( oneSided || (slowData.mean() / factor < (fastData.mean() + num_sds*fastSD)) )
            && slowData.mean() / factor >(fastData.mean() - num_sds*fastSD);
        }
      }

      if(!results.passed)
      {
        std::ostringstream error;
        error << logger.failure_description(description);
        error << "\n\tFast Task duration: " << fastData.mean() << "s";
        error << " +- " << num_sds << " * " << fastData.template sample_standard_deviation<gaussian_approx_modifier>();
        error << "\n\tSlow Task duration: " << slowData.mean() << "s";
        error << " +- " << num_sds << " * " << slowData.template sample_standard_deviation<gaussian_approx_modifier>();
        error << " (" << slowData.mean() / fastData.mean() << "; " << factor << ")";
        logger.log_failure(error.str());
      }
      else if(reportSuccess)
      {
        std::ostringstream message;
        message << "\n\tFast Task duration: " << fastData.mean() << "s";
        message << " +- " << num_sds << " * " << fastData.template sample_standard_deviation<gaussian_approx_modifier>();
        message << "\n\tSlow Task duration: " << slowData.mean() << "s";
        message << " +- " << num_sds << " * " << slowData.template sample_standard_deviation<gaussian_approx_modifier>();;
        logger.post_message(message.str());
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

      template<class T> bool check_equality(const T& reference, const T& value, const std::string& description="")
      {
        return unit_testing::check_equality(m_Logger, reference, value, description);
      }

      template<class T>
      bool check_equality_within_tolerance(const T reference, const T value, const T tol, const std::string& description="")
      {
        return unit_testing::check_equality_within_tolerance(m_Logger, reference, value, tol, description);
      }

      bool check(const bool value, const std::string& description="")
      {
        return unit_testing::check_equality(m_Logger, true, value, description);
      }

      template<class E, class Fn>
      bool check_exception_thrown(Fn&& function, const std::string& description="")
      {
        return unit_testing::check_exception_thrown<Logger, E, Fn>(m_Logger, std::forward<Fn>(function), description);
      }

      template<class RefIter, class ResultIter>
      bool check_range(RefIter refBegin, RefIter refEnd, ResultIter resBegin, ResultIter resEnd, const std::string& description="")
      {
        return unit_testing::check_range(m_Logger, refBegin, refEnd, resBegin, resEnd, description);
      }

      template<class F, class S, std::size_t N=4>
      auto check_relative_performance(F fast, S slow, const double factor, const bool oneSided, const std::string& description="", const std::size_t trials=5, const double num_sds=3)
        -> performance_results<decltype(fast())>
      {
        return unit_testing::check_relative_performance(m_Logger, fast, slow, factor, oneSided, trials, num_sds, false, description);
      }

      template<class Stream>
      friend Stream& operator<<(Stream& os, const checker& checker)
      {
        os << checker.m_Logger;
        return os;
      }

      log_summary summary(const std::string& prefix) const
      {
        return log_summary{prefix, m_Logger};
      }
    protected:
      checker(checker&&)            = default;
      checker& operator=(checker&&) = default;

      ~checker() = default;

      void log_critical_failure(const std::string& message) { m_Logger.log_critical_failure(message); }
      void log_failure(const std::string& message) { m_Logger.log_failure(message); }
      std::size_t checks() const noexcept { return m_Logger.checks(); }
      std::size_t failures() const noexcept { return m_Logger.failures(); }
      void failure_message_prefix(std::string_view prefix) { m_Logger.failure_message_prefix(prefix); }
      std::string failure_message_prefix() const { return m_Logger.failure_message_prefix(); }
      void post_message(const std::string& message) { m_Logger.post_message(message); }
      std::string current_message() const { return m_Logger.current_message(); }
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

      std::string name() const { return m_Name; }
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

        return Checker::summary("  " + name() + ":\n");
      }
    protected:
      virtual void run_tests() = 0;

      virtual std::string current_message() const { return Checker::failure_message_prefix() + Checker::current_message(); }

      std::string make_message(std::string_view tag, std::string_view exceptionMessage) const
      {
        std::string mess{"\tError -- " + std::string(tag) + " Exception:" + format(exceptionMessage) + '\n'};
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
        return s.empty() ? std::string{s} : '\t' + std::string{s};
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

      log_summary execute()
      {
        log_summary summary{m_Name + ":\n"};
        for(auto& pTest : m_Tests)
        {
          summary += pTest->execute();
        }

        return summary;
      }
      
      std::string name() { return m_Name; }
      
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

    template<class T> struct type_to_string
    {
      static std::string str();
    };

    template<bool B> struct bool_to_string
    {
      static std::string str() { return "TRUE"; }
    };

    template<> struct bool_to_string<false>
    {
      static std::string str() { return "FALSE"; }
    };

    template<template <class...> class T> struct template_class_to_string
    {
      static std::string str();
    };

    inline std::string report_line(std::string file, const int line, const std::string_view message)
    {
      std::string s{std::move(file) + ", Line " + std::to_string(line)};
      if(!message.empty()) (s += ": ") += message;

      return s;
    }

#define LINE(message) report_line(__FILE__, __LINE__, message)
  }
}
