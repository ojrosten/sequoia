////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file PerformanceTestCore.hpp
    \brief Extension of unit testing framework for perfomance testing
*/

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  template<class R> struct performance_results
  {
    std::vector<std::future<R>> fast_futures, slow_futures;
    bool passed{};
  };

  template<class Logger, class F, class S>
  [[nodiscard]]
  auto check_relative_performance(std::string_view description, Logger& logger, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials, const double num_sds) -> performance_results<std::invoke_result_t<F>>
  {      
    using R = std::invoke_result_t<F>;
    static_assert(std::is_same_v<R, std::invoke_result_t<S>>, "Fast/Slow invokables must have same return value");

    if((minSpeedUp <= 1) || (maxSpeedUp <= 1))
      throw std::logic_error("Relative performance test requires speed-up factors > 1!");

    if(minSpeedUp > maxSpeedUp)
      throw std::logic_error("maxSpeedUp must be >= minSpeedUp");      
      
    performance_results<R> results;      
      
    using namespace std::chrono;
    using namespace maths;

    std::string
      message{description.empty() ? "" : std::string{"\t"}.append(description).append("\n")},
      summary{};  

      std::size_t remainingAttempts{2};

      while(remainingAttempts > 0)
      {
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
            std::ostringstream message{};
            message << "\tFast Task duration: " << m_f << "s";
            message << " +- " << num_sds << " * " << sig_f;
            message << "\n\tSlow Task duration: " << m_s << "s";
            message << " +- " << num_sds << " * " << sig_s;
            message << " [" << m_s / m_f << "; (" << minSpeedUp << ", " << maxSpeedUp << ")]\n";

            return message.str();
          }
        };
        
        summary = serializer();

        if((Logger::mode == test_mode::false_positive) ? !results.passed : results.passed)
        {
          break;
        }
        
        --remainingAttempts;
        if(remainingAttempts)
          results = performance_results<R>{};      
      }

      message.append(summary);

      typename Logger::sentinel r{logger, message};
      r.log_performance_check();

      if(!results.passed)
      {        
        logger.log_performance_failure(message);
      }

      return results;
  }

  template<class Logger>
  class performance_extender
  {
  public:
    performance_extender(Logger& logger) : m_Logger{logger} {}

    performance_extender(const performance_extender&) = delete;
    performance_extender(performance_extender&&)      = delete;

    performance_extender& operator=(const performance_extender&) = delete;    
    performance_extender& operator=(performance_extender&&)      = delete;
 
    template<class F, class S>
    auto check_relative_performance(std::string_view description, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials=5, const double num_sds=3)
      -> performance_results<std::invoke_result_t<F>>
    {
      return unit_testing::check_relative_performance(description, m_Logger, fast, slow, minSpeedUp, maxSpeedUp, trials, num_sds);
    }
  protected:
    ~performance_extender() = default;

  private:
    Logger& m_Logger;
  };

  template<test_mode mode>
  using performance_checker = checker<unit_test_logger<mode>, performance_extender<unit_test_logger<mode>>>;
  
  template<test_mode mode>
  using basic_performance_test = basic_test<unit_test_logger<mode>, performance_checker<mode>>;

  using performance_test                = basic_performance_test<test_mode::standard>;
  using performance_false_negative_test = basic_performance_test<test_mode::false_negative>;
  using performance_false_positive_test = basic_performance_test<test_mode::false_positive>;
}
