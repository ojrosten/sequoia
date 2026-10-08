////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include <array>
#include <chrono>
#include <memory>

namespace sequoia::testing
{
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

  /** \brief Returns a task which spins for each of `durations` in turn,
             starting again after the last.

      Every copy of the task shares one count of the calls made, so any
      five consecutive calls spin for each duration once.
   */
  [[nodiscard]]
  inline auto make_cycling_spinner(const std::array<std::chrono::milliseconds, 5>& durations)
  {
    return [durations, calls{std::make_shared<std::size_t>()}]() {
      spin_for(durations[(*calls)++ % durations.size()]);
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
               calls, and for `later` on every call after them.
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
