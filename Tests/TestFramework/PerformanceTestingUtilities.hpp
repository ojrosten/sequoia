////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <format>
#include <functional>
#include <initializer_list>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace sequoia::testing
{
  /** \brief The number of trials in each attempt of
             `check_relative_performance`, when `minimum_trials` is 10 and
             when it is 11.

      The numbers are not in the contract. Tests which use them pin the
      implementation's choice, and change with it.
   */
  inline constexpr auto trials_per_attempt_for_minimum_10{std::to_array<std::size_t>({10, 16, 20})},
                        trials_per_attempt_for_minimum_11{std::to_array<std::size_t>({12, 16, 22})};

  static_assert(trials_per_attempt_for_minimum_10.size() == relative_performance_max_attempts);
  static_assert(trials_per_attempt_for_minimum_11.size() == relative_performance_max_attempts);
  static_assert(trials_per_attempt_for_minimum_10.front() == relative_performance_minimum_trials_floor);

  /** \brief Returns the number of trials made before the `n`th attempt,
             given the number in each attempt.

      Attempts are numbered from 1, and `n` may name the attempt after the
      last, which gives the trials of every attempt.

      \throws std::out_of_range if `n` is zero or exceeds
      `trialsPerAttempt.size() + 1`.
   */
  [[nodiscard]]
  constexpr std::size_t trials_before_nth_attempt(std::span<const std::size_t> trialsPerAttempt, std::size_t n)
  {
    if((n == 0) || (n > trialsPerAttempt.size() + 1))
      throw std::out_of_range{"An attempt number must lie between 1 and one past the number of attempts"};

    return std::ranges::fold_left(trialsPerAttempt | std::views::take(n - 1), 0uz, std::plus<>{});
  }

  /** \brief Returns `numTrials` trials' durations: those of `atypical`,
             followed by `typical` for the rest.

      \throws std::logic_error if `atypical` has more elements than
      `numTrials`.
   */
  template<class Durations>
  [[nodiscard]]
  std::vector<Durations> trials_with_atypical(std::size_t numTrials,
                                              Durations typical,
                                              std::initializer_list<Durations> atypical)
  {
    if(atypical.size() > numTrials)
      throw std::logic_error{
        std::format("More atypical trials ({}) than trials ({})", atypical.size(), numTrials)
      };

    auto durationsOfTrial{
      [typical, atypical](std::size_t trial) { return trial < atypical.size() ? atypical.begin()[trial] : typical; }
    };

    return std::views::iota(0uz, numTrials) | std::views::transform(durationsOfTrial) | std::ranges::to<std::vector>();
  }

  /** \brief Keeps the calling thread busy until `t` has passed by the
             steady clock.

      A sleep ends on the operating system's timer, and so overruns by an
      amount that varies with the platform and the load. That distorts the
      ratio of two tasks' durations. A spin ends at the first reading of the
      clock past its deadline.
   */
  inline void spin_for(std::chrono::microseconds t)
  {
    const auto deadline{std::chrono::steady_clock::now() + t};
    while(std::chrono::steady_clock::now() < deadline) {}
  }

  /** \brief Returns the state of Marsaglia's 64-bit xorshift generator
             after `steps` steps from `seed`.

      Each step depends on the one before, so the computation takes time in
      proportion to `steps`.
   */
  [[nodiscard]]
  constexpr std::uint64_t xorshift_state(std::uint64_t seed, std::size_t steps) noexcept
  {
    auto nextState{
      [](std::uint64_t state, std::size_t) {
        state ^= state << 13;
        state ^= state >> 7;
        return state ^ (state << 17);
      }
    };

    return std::ranges::fold_left(std::views::iota(0uz, steps), seed, nextState);
  }

  /** \brief Returns a task which spins for each of `durations` in turn,
             starting again after the last.

      Every copy of the task shares one count of the calls made, so any
      `N` consecutive calls spin for each duration once.
   */
  template<std::size_t N>
  [[nodiscard]]
  auto make_cycling_spinner(const std::chrono::milliseconds (&durations)[N])
  {
    return [cycle{std::to_array(durations)}, calls{std::make_shared<std::size_t>()}]() {
      spin_for(cycle[(*calls)++ % N]);
    };
  }

  /** \brief A task which spins, and counts its invocations.

      Every copy of the task shares one count of the invocations, so the
      count covers every trial of every attempt.
   */
  class counted_spinner
  {
  public:
    /** \brief Spins for `duration` on every call. */
    explicit counted_spinner(std::chrono::microseconds duration)
      : counted_spinner{duration, 0, duration}
    {}

    /** \brief Spins for `initial` on each of the first `initialCalls`
               calls, and for `later` on every call thereafter.
     */
    counted_spinner(std::chrono::microseconds initial,
                    std::size_t initialCalls,
                    std::chrono::microseconds later)
      : m_Initial{initial}
      , m_Later{later}
      , m_InitialCalls{initialCalls}
    {}

    void operator()() const
    {
      spin_for((*m_Calls)++ < m_InitialCalls ? m_Initial : m_Later);
    }

    [[nodiscard]]
    std::size_t calls() const noexcept { return *m_Calls; }
  private:
    std::chrono::microseconds m_Initial{}, m_Later{};
    std::size_t m_InitialCalls{};
    std::shared_ptr<std::size_t> m_Calls{std::make_shared<std::size_t>()};
  };

  /** \brief A clock whose reading changes only through `advance`. */
  class scripted_clock
  {
  public:
    using duration   = std::chrono::nanoseconds;

    // Not std::chrono::time_point<scripted_clock>: the standard requires a
    // time_point's clock to meet Cpp17Clock, which needs a static now().
    using time_point = std::chrono::time_point<std::chrono::steady_clock, duration>;

    [[nodiscard]]
    time_point now() const noexcept { return m_Now; }

    void advance(duration d) noexcept { m_Now += d; }
  private:
    time_point m_Now{};
  };

  /** \brief Returns a task which advances `clock` by each of `durations` in
             turn, starting again after the last.

      Every copy of the task shares one count of the calls made, so any
      `N` consecutive calls advance `clock` by each duration once.
   */
  template<std::size_t N>
  [[nodiscard]]
  auto make_cycling_scripted_task(scripted_clock& clock, const scripted_clock::duration (&durations)[N])
  {
    return [&clock, cycle{std::to_array(durations)}, calls{std::make_shared<std::size_t>()}]() {
      clock.advance(cycle[(*calls)++ % N]);
    };
  }

  /** \brief A task which advances a `scripted_clock`, and counts its
             invocations.

      Every copy of the task shares one count of the invocations, so the
      count covers every trial of every attempt.
   */
  class counted_scripted_task
  {
  public:
    /** \brief Advances `clock` by `duration` on every call. */
    counted_scripted_task(scripted_clock& clock, scripted_clock::duration duration)
      : counted_scripted_task{clock, duration, 0, duration}
    {}

    /** \brief Advances `clock` by `initial` on each of the first
               `initialCalls` calls, and by `later` on every call thereafter.
     */
    counted_scripted_task(scripted_clock& clock,
                          scripted_clock::duration initial,
                          std::size_t initialCalls,
                          scripted_clock::duration later)
      : m_Clock{&clock}
      , m_Initial{initial}
      , m_Later{later}
      , m_InitialCalls{initialCalls}
    {}

    void operator()() const
    {
      m_Clock->advance((*m_Calls)++ < m_InitialCalls ? m_Initial : m_Later);
    }

    [[nodiscard]]
    std::size_t calls() const noexcept { return *m_Calls; }
  private:
    scripted_clock* m_Clock{};
    scripted_clock::duration m_Initial{}, m_Later{};
    std::size_t m_InitialCalls{};
    std::shared_ptr<std::size_t> m_Calls{std::make_shared<std::size_t>()};
  };

  /** \brief A test whose performance checks are meant to read a
             `scripted_clock`.

      The test is not a performance test, so it may run alongside other
      tests. A performance check which reads a real clock belongs in a
      performance test.
   */
  template<test_mode Mode>
  using basic_scripted_clock_test = basic_test<Mode, performance_extender<Mode>>;

  using scripted_clock_test                = basic_scripted_clock_test<test_mode::standard>;
  using scripted_clock_false_negative_test = basic_scripted_clock_test<test_mode::false_negative>;
  using scripted_clock_false_positive_test = basic_scripted_clock_test<test_mode::false_positive>;
}
