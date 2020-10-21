////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Extension of the testing framework for perfomance testing.
*/

#include "RegularTestCore.hpp"
#include "StatisticalAlgorithms.hpp"

#include <chrono>
#include <random>
#include <future>

namespace sequoia::testing
{
  /*! \brief Function for comparing the performance of a fast task to a slow task.

       \param minSpeedUp  the minimum predicted speed up of fast over slow; must be > 1
       \param maxSpeedUp  the maximum predicted speed up of fast over slow; must be > minSpeedUp
       \param trials      the number of trial used for the statistical analysis
       \param num_sds     the number of standard deviations used to define a significant result
       \param maxAttempts the number of times the entire test should be re-run before accepting failure

       For each trial, both the supposedly fast and slow tasks are run. Their order is random.
       When all trials have been completed, the mean and standard deviations are computed for
       both fast and slow tasks. Denote these by m_f, sig_f and m_s, sig_s.

       if (m_f + sig_f < m_s + sig_s)

       then it is concluded that the purportedly fast task is actually slower than the slow task and
       so the test fails. If this is not the case then the analysis branches depending on which
       standard deviation is bigger.

       if (sig_f >= sig_s)

       then we mutliply m_f by both the min/max predicted speed-up and compare to the range of
       values around m_s defined by the number of standard deviations. In particular, the test
       is taken to pass if
       
          (minSpeedUp * m_f <= (m_s + num_sds * sig_s))
       && (maxSpeedUp * m_f >= (m_s - num_sds * sig_s))
       
       which is essentially saying that the range of predicted speed-ups must fall within 
       the specified number of standard deviations of m_s.

       On the other hand

       if(sig_s > sif_g)

       then we divide m_s by both  the min/max predicted speed-up and compare to the range of
       values around m_f defined by the number of standard deviations. In particular, the test
       is taken to pass if

          (m_s / maxSpeedUp <= (m_f + num_sds * sig_f))
       && (m_s / minSpeedUp >= (m_f - num_sds * sig_f))
       
   */
  template<test_mode Mode, class F, class S>
  bool check_relative_performance(std::string_view description, test_logger<Mode>& logger, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials, const double num_sds, const std::size_t maxAttempts)
  {
    if((minSpeedUp <= 1) || (maxSpeedUp <= 1))
      throw std::logic_error{"Relative performance test requires speed-up factors > 1"};

    if(minSpeedUp > maxSpeedUp)
      throw std::logic_error{"maxSpeedUp must be >= minSpeedUp"};

    if(num_sds <=1)
      throw std::logic_error{"Number of standard deviations is required to be > 1"};

    if(!maxAttempts)
      throw std::logic_error{"Number of attempts is required to be > 0"};

    if(trials < 5)
      throw std::logic_error{"Number of trials is required to be > 4"};

    using namespace std::chrono;
    using namespace maths;

    std::string summary{};  
    std::size_t remainingAttempts{maxAttempts};
    bool passed{};

    auto timer{
       [](auto task, std::vector<double>& timings){
         const auto start{steady_clock::now()};
         task();
         const auto end{steady_clock::now()};

         const duration<double> duration = end - start;
         timings.push_back(duration.count());
       }
    };

    while(remainingAttempts > 0)
    {
      const auto adjustedTrials{trials*(maxAttempts - remainingAttempts + 1)};

      std::vector<double> fastData, slowData;
      fastData.reserve(adjustedTrials);
      slowData.reserve(adjustedTrials);
        
      std::random_device generator;
      for(std::size_t i{}; i < adjustedTrials; ++i)
      {
        std::uniform_real_distribution<double> distribution{0.0, 1.0};
        const bool fastFirst{(distribution(generator) < 0.5)};

        if(fastFirst)
        {
          timer(fast, fastData);
          timer(slow, slowData);
        }
        else
        {
          timer(slow, slowData);
          timer(fast, fastData);
        }
      }

      auto compute_stats{
        [](auto first, auto last) {
          const auto data{sample_standard_deviation(first, last)};
          return std::make_pair(data.first.value(), data.second.value());
        }
      };

      std::sort(fastData.begin(), fastData.end());
      std::sort(slowData.begin(), slowData.end());
      
      const auto [sig_f, m_f]{compute_stats(fastData.cbegin()+1, fastData.cend()-1)};
      const auto [sig_s, m_s]{compute_stats(slowData.cbegin()+1, slowData.cend()-1)};

      if(m_f + sig_f < m_s - sig_s)
      {
        if(sig_f >= sig_s)
        {
          passed =    (minSpeedUp * m_f <= (m_s + num_sds * sig_s))
                   && (maxSpeedUp * m_f >= (m_s - num_sds * sig_s));
        }
        else
        {
          passed =    (m_s / maxSpeedUp <= (m_f + num_sds * sig_f))
                   && (m_s / minSpeedUp >= (m_f - num_sds * sig_f));
        }
      }
      else
      {
        passed = false;
      }

      auto stats{
        [num_sds](std::string_view prefix, const auto mean, const auto sig){
            
          std::ostringstream message{};
          message << mean << "s" << " +- " << num_sds << " * " << sig;

          return std::string{prefix}.append(" Task duration: ").append(message.str());
        }
      };

      auto summarizer{
        [m_f{m_f},m_s{m_s},minSpeedUp,maxSpeedUp](){
          std::ostringstream message{};
          message << " [" << m_s / m_f << "; (" << minSpeedUp << ", " << maxSpeedUp << ")]";

          return message.str();
        }
      };
        
      summary = append_lines(stats("Fast", m_f, sig_f), stats("Slow", m_s, sig_s)).append(summarizer());
          
      if((test_logger<Mode>::mode == test_mode::false_positive) ? !passed : passed)
      {
        break;
      }
        
      --remainingAttempts;
    }

    sentinel<Mode> sentry{logger, append_lines(description, summary)};
    sentry.log_performance_check();

    if(!passed)
    {        
      sentry.log_performance_failure("");
    }

    return passed;
  }

  /*! \brief class template for plugging into the checker class template
      \anchor performance_extender_primary
   */
  template<test_mode Mode>
  class performance_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    performance_extender(test_logger<Mode>& logger) : m_Logger{logger} {}

    performance_extender(const performance_extender&) = delete;
    performance_extender(performance_extender&&)      = delete;

    performance_extender& operator=(const performance_extender&) = delete;    
    performance_extender& operator=(performance_extender&&)      = delete;
 
    template<class F, class S>
    bool check_relative_performance(std::string_view description, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials=5, const double num_sds=4)
    {
      return testing::check_relative_performance(description, m_Logger, fast, slow, minSpeedUp, maxSpeedUp, trials, num_sds, 3);
    }
  protected:
    ~performance_extender() = default;

  private:
    test_logger<Mode>& m_Logger;
  };

  template<test_mode mode>
  using performance_checker = checker<mode, performance_extender<mode>>;
  
  template<test_mode mode>
  using basic_performance_test = basic_test<performance_checker<mode>>;

  /*! \anchor performance_test_alias */
  using performance_test                = basic_performance_test<test_mode::standard>;
  using performance_false_negative_test = basic_performance_test<test_mode::false_negative>;
  using performance_false_positive_test = basic_performance_test<test_mode::false_positive>;
}
