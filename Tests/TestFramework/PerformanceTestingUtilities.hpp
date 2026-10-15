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
             `check_relative_performance`, for minima of 10 and 11.

      The numbers are not in the contract. Tests which use them pin the
      implementation's choice, and change with it.
   */
  inline constexpr auto trials_for_minimum_10{std::to_array<std::size_t>({10, 16, 20})},
                        trials_for_minimum_11{std::to_array<std::size_t>({12, 16, 22})};

  static_assert(trials_for_minimum_10.size() == relative_performance_max_attempts);
  static_assert(trials_for_minimum_11.size() == relative_performance_max_attempts);
  static_assert(trials_for_minimum_10.front() == relative_performance_minimum_trials_floor);

  /** \brief Returns the number of trials made before attempt `attempt`,
             given the number in each attempt.
   */
  [[nodiscard]]
  constexpr std::size_t trials_before_attempt(std::span<const std::size_t> trialsOfAttempts, std::size_t attempt)
  {
    return std::ranges::fold_left(trialsOfAttempts | std::views::take(attempt - 1), 0uz, std::plus<>{});
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
  inline void spin_for(std::chrono::milliseconds t)
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
    explicit counted_spinner(std::chrono::milliseconds duration)
      : counted_spinner{duration, 0, duration}
    {}

    /** \brief Spins for `initial` on each of the first `initialCalls`
               calls, and for `later` on every call thereafter.
     */
    counted_spinner(std::chrono::milliseconds initial,
                    std::size_t initialCalls,
                    std::chrono::milliseconds later)
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
    std::chrono::milliseconds m_Initial{}, m_Later{};
    std::size_t m_InitialCalls{};
    std::shared_ptr<std::size_t> m_Calls{std::make_shared<std::size_t>()};
  };
}
