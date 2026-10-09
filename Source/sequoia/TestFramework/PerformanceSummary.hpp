////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The text of a relative performance check's summary, and the
           zeroing of the values in it which vary from run to run.
 */

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>

namespace sequoia::testing
{
  /** \brief Why `check_relative_performance` judged that the speed-up of a
             fast task over a slow one departs from the prediction.
   */
  enum class relative_performance_failure
  {
    not_distinguishably_faster,
    slower,
    faster_but_less_than_predicted,
    suspiciously_fast
  };

  /** \brief A closed interval of speed-ups. */
  struct relative_performance_interval
  {
    double lower{}, upper{};
  };

  /** \brief A measured speed-up, and the interval around it. */
  struct relative_performance_estimate
  {
    double                        speedup{};
    relative_performance_interval interval{};
  };

  /** \brief Which attempt this is, and how many are allowed. */
  struct relative_performance_attempts
  {
    std::size_t current{}, maximum{};
  };

  /** \brief A duration for each of the fast and slow tasks. */
  struct relative_performance_durations
  {
    std::chrono::duration<double> fast{}, slow{};
  };

  /** \brief Returns a line stating why the speed-up departs from the
             prediction.
   */
  [[nodiscard]]
  std::string_view verdict_summary(relative_performance_failure failure);

  /** \brief Returns a line reporting the measured speed-up, the interval
             around it, and the interval predicted for it.
   */
  [[nodiscard]]
  std::string speedup_summary(const relative_performance_estimate& obtained, relative_performance_interval prediction);

  /** \brief Returns a line reporting the number of trials, which attempt
             made them, and how many attempts are allowed.
   */
  [[nodiscard]]
  std::string trials_summary(std::size_t trials, relative_performance_attempts attempts);

  /** \brief Returns a line reporting the duration of each task. */
  [[nodiscard]]
  std::string task_durations_summary(relative_performance_durations durations);

  /** \brief Returns `text` with the values in its summary lines which vary
             from run to run set to zero.

      A summary line is one which `speedup_summary`, `trials_summary` or
      `task_durations_summary` reprints exactly from the numbers it contains,
      once any leading spaces and tabs are set aside. The leading spaces and
      tabs stay as they are. The values set to zero are:
      -# The measurements: the speed-up, the interval around it and the task
         durations;
      -# The attempt to which the measurements led, and its number of trials.

      The predicted interval and the number of attempts allowed stay as they
      are, and so does every other line. So the results for two texts are
      equal if and only if the texts differ only in the values set to zero.
   */
  [[nodiscard]]
  std::string text_with_zeroed_measurements(std::string_view text);
}
