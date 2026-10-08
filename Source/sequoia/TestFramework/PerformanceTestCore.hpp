////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Extension of the testing framework for performance testing.
*/

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Statistics/StatisticalAlgorithms.hpp"
#include "sequoia/TestFramework/FileEditors.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <random>
#include <ranges>

namespace sequoia::testing
{
  template<std::invocable Task>
  [[nodiscard]]
  std::chrono::duration<double> profile(Task task)
  {
    const timer t{};
    task();

    return t.time_elapsed();
  }

  /** \brief Function for comparing the performance of a fast task to a slow task.

       \param description the description reported with the check
       \param logger      the logger to which the result is reported
       \param fast        the task predicted to be the faster of the two
       \param slow        the task against which fast is compared
       \param minSpeedUp  the minimum predicted speed up of fast over slow; must be > 1
       \param maxSpeedUp  the maximum predicted speed up of fast over slow; must be >= minSpeedUp
       \param trials      the number of trials used for the statistical analysis
       \param num_sds     the number of standard deviations used to define a significant result
       \param maxAttempts the number of times the entire test should be re-run before accepting failure

       For each trial, both the supposedly fast and slow tasks are run. Their order is random.
       When all trials have been completed, the mean and standard deviations are computed for
       both fast and slow tasks. Denote these by m_f, sig_f and m_s, sig_s.

       The test fails unless

          m_f + sig_f < m_s - sig_s

       that is, unless the fast task is faster by more than the sum of the standard deviations.
       If it is, the analysis branches depending on which standard deviation is bigger.

       if (sig_f >= sig_s)

       then we multiply m_f by both the min/max predicted speed-up and compare to the range of
       values around m_s defined by the number of standard deviations. In particular, the test
       is taken to pass if

          (minSpeedUp * m_f <= (m_s + num_sds * sig_s))
       && (maxSpeedUp * m_f >= (m_s - num_sds * sig_s))

       which is essentially saying that the range of predicted speed-ups must fall within
       the specified number of standard deviations of m_s.

       On the other hand

       if (sig_s > sig_f)

       then we divide m_s by both the min/max predicted speed-up and compare to the range of
       values around m_f defined by the number of standard deviations. In particular, the test
       is taken to pass if

          (m_s / maxSpeedUp <= (m_f + num_sds * sig_f))
       && (m_s / minSpeedUp >= (m_f - num_sds * sig_f))

       \throws std::invalid_argument if a speed-up factor is not greater than 1,
       if minSpeedUp exceeds maxSpeedUp, if num_sds is not greater than 1, if
       maxAttempts is 0 or if trials is less than 5.
   */
  template<test_mode Mode, std::invocable F, std::invocable S>
  bool check_relative_performance(std::string_view description, test_logger<Mode>& logger, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials, const double num_sds, const std::size_t maxAttempts)
  {
    sentinel<Mode> sentry{logger, std::string{description}};
    sentry.log_performance_check();

    if(!(minSpeedUp > 1) || !(maxSpeedUp > 1))
      throw std::invalid_argument{"Relative performance test requires speed-up factors > 1"};

    if(minSpeedUp > maxSpeedUp)
      throw std::invalid_argument{"maxSpeedUp must be >= minSpeedUp"};

    if(!(num_sds > 1))
      throw std::invalid_argument{"Number of standard deviations is required to be > 1"};

    if(!maxAttempts)
      throw std::invalid_argument{"Number of attempts is required to be > 0"};

    if(trials < 5)
      throw std::invalid_argument{"Number of trials is required to be > 4"};

    using namespace std::chrono;
    using namespace maths;

    std::string summary{};
    std::size_t remainingAttempts{maxAttempts};
    bool passed{};

    auto timer{
       [](auto task, std::vector<double>& timings){
         timings.push_back(profile(task).count());
       }
    };

    while(remainingAttempts > 0)
    {
      const auto adjustedTrials{trials*(maxAttempts - remainingAttempts + 1)};

      std::vector<double> fastData{}, slowData{};
      fastData.reserve(adjustedTrials);
      slowData.reserve(adjustedTrials);

      std::random_device generator{};
      for([[maybe_unused]] auto _ : std::views::iota(0uz, adjustedTrials))
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

      std::ranges::sort(fastData);
      std::ranges::sort(slowData);

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
          return std::format("{} Task duration: {:g}s +- {:g} * {:g}s", prefix, mean, num_sds, sig);
        }
      };

      auto summarizer{
        [m_f, m_s, minSpeedUp, maxSpeedUp](){
          return std::format(" [{:g}; ({:g}, {:g})]", m_s / m_f, minSpeedUp, maxSpeedUp);
        }
      };

      summary = append_lines(stats("Fast", m_f, sig_f), stats("Slow", m_s, sig_s)).append(summarizer());

      if((test_logger<Mode>::mode == test_mode::false_negative) ? !passed : passed)
      {
        break;
      }

      --remainingAttempts;
    }

    sentry.append_to_message(summary);

    if(!passed)
    {
      sentry.log_performance_failure("");
    }

    return passed;
  }

  /** \brief Whether `slept`, compared to `target`, indicates sleeps rounded up to a coarse timer tick. */
  [[nodiscard]]
  bool is_coarse_sleep(std::chrono::duration<double, std::milli> slept,
                       std::chrono::duration<double, std::milli> target);

  /** \brief A warning that sleeps of `target` lasted `slept` or more, so timings built on sleeps are unreliable. */
  [[nodiscard]]
  std::string coarse_sleep_message(std::chrono::duration<double, std::milli> slept,
                                   std::chrono::duration<double, std::milli> target);

  /** \brief class template for plugging into the checker class template
      \anchor performance_extender_primary
   */
  template<test_mode Mode>
  class performance_extender
  {
  public:
    constexpr static test_mode mode{Mode};

    performance_extender() = default;

    template<class Self, std::invocable F, std::invocable S>
    bool check_relative_performance(this Self& self, const reporter& description, F fast, S slow, const double minSpeedUp, const double maxSpeedUp, const std::size_t trials=5, const double num_sds=4)
    {
      return testing::check_relative_performance(self.report(description), self.m_Logger, fast, slow, minSpeedUp, maxSpeedUp, trials, num_sds, 3);
    }
  protected:
    ~performance_extender() = default;

    performance_extender(performance_extender&&)            noexcept = default;
    performance_extender& operator=(performance_extender&&) noexcept = default;
  };

  /** \brief Chooses between a run's diagnostics output and the reference.

      \returns
      -# `referenceOutput`, if every difference from `testOutput` lies after
         the `Task duration:` label on its line, and leaves the number of
         standard deviations and the range of speed-ups unchanged;
      -# `testOutput`, otherwise.
   */
  [[nodiscard]]
  std::string_view postprocess(std::string_view testOutput, std::string_view referenceOutput);

  /**\brief class template from which all concrete tests should derive */

  template<test_mode Mode>
  class basic_performance_test : public basic_test<Mode, performance_extender<Mode>>
  {
  public:
    using base_type = basic_test<Mode, performance_extender<Mode>>;
    using duration  = base_type::duration;

    using base_type::base_type;

    [[nodiscard]]
    log_summary summarize(duration delta) const;
  protected:
    ~basic_performance_test() = default;

    basic_performance_test(basic_performance_test&&)            noexcept = default;
    basic_performance_test& operator=(basic_performance_test&&) noexcept = default;
  };

  /** \anchor performance_test_alias */
  using performance_test                = basic_performance_test<test_mode::standard>;
  using performance_false_positive_test = basic_performance_test<test_mode::false_positive>;
  using performance_false_negative_test = basic_performance_test<test_mode::false_negative>;

  template<concrete_test T>
  inline constexpr bool is_performance_test_v{std::derived_from<T, basic_performance_test<T::mode>>};

  template<concrete_test T>
    requires is_performance_test_v<T>
  struct is_parallelizable<T> : std::false_type {};
}
