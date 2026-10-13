////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The record of an execution, which the test runner keeps for each
           test, and for each run as the run's stamp.
 */

#include <chrono>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief What a record adds once its execution has finished.

      A completion gives the execution duration, the runner's overhead, and
      the number of attempts each performance check made. A performance check
      which threw before its last attempt finished has no entry.
   */
  struct execution_completion
  {
    std::chrono::microseconds execution_duration{},
                              runner_overhead{};
    std::vector<std::size_t>  performance_check_attempts{};

    [[nodiscard]]
    friend bool operator==(const execution_completion&, const execution_completion&) noexcept = default;
  };

  /** \brief The start of an execution, to the millisecond, and its completion
             once the execution has finished.

      `operator>>` reads back what `operator<<` writes, for a start in the
      years 0 to 9999 and durations which are not negative.
   */
  struct execution_record
  {
    using start_type = std::chrono::sys_time<std::chrono::milliseconds>;

    start_type start{};
    std::optional<execution_completion> completion{};

    [[nodiscard]]
    friend bool operator==(const execution_record&, const execution_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const execution_record& record);

    /** \brief Reads a record which runs to the end of `s`.

        At the end of `s`, sets `failbit` and leaves `record` unchanged.

        \throws std::runtime_error if the text is not a record, or gives a
                negative duration; `record` is then unchanged
     */
    friend std::istream& operator>>(std::istream& s, execution_record& record);
  };

  [[nodiscard]]
  std::string to_string(const execution_record& record);

  /** \brief Reads the execution record held by `file`.

      \throws std::runtime_error if `file` cannot be opened, or does not hold
              a record
   */
  [[nodiscard]]
  execution_record read_execution_record(const std::filesystem::path& file);
}
