////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The text of a relative performance check's summary, and how the
           measured values in it are recognised.
 */

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

  /** \brief Returns a line stating why the speed-up departs from the
             prediction.
   */
  [[nodiscard]]
  std::string_view verdict_summary(relative_performance_failure failure);

  /** \brief Returns a line reporting the measured speed-up, the interval
             around it, and the range predicted for it.
   */
  [[nodiscard]]
  std::string speedup_summary(double speedup,
                              double intervalMin,
                              double intervalMax,
                              double minSpeedup,
                              double maxSpeedup);

  /** \brief Returns a line reporting the number of trials, and which
             attempt made them.
   */
  [[nodiscard]]
  std::string trials_summary(std::size_t trials, std::size_t attempt, std::size_t maxAttempts);

  /** \brief Returns a line reporting the typical duration of each task, in
             seconds.
   */
  [[nodiscard]]
  std::string task_durations_summary(double fastDuration, double slowDuration);

  /** \brief Returns `text` with the measured values in each of its lines set
             to zero.

      The measured values are the speed-up and the interval around it, the
      number of trials and the attempt which made them, and the task durations.
      A line holds them only if `speedup_summary`, `trials_summary` or
      `task_durations_summary` reprints it exactly from the numbers it
      contains. The predicted range and the number of attempts allowed stay as
      they are. Any other line is unchanged.
   */
  [[nodiscard]]
  std::string text_with_zeroed_measurements(std::string_view text);
}
