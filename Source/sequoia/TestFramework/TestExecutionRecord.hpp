////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief The record of a test's execution and the stamp of a run of tests,
           and the functions which write and read them.
 */

#include <chrono>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace sequoia::testing
{
  /** \brief What a test's record gains once the test has finished.

      `performance_check_attempts` gives the number of attempts each
      performance check made. A check which threw before its last attempt
      finished has no entry.

      `operator>>` reads back what `operator<<` writes, provided that no
      duration is negative.
   */
  struct test_execution_completion
  {
    std::chrono::microseconds execution_duration{},
                              runner_overhead{};
    std::vector<std::size_t>  performance_check_attempts{};

    [[nodiscard]]
    friend bool operator==(const test_execution_completion&, const test_execution_completion&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const test_execution_completion& completion);

    /** \brief Reads a completion which runs from the read position of `s`
               to the end of `s`.

        A completion has no end marker: its entries of attempts run until
        the stream ends. So nothing can follow a completion in `s`.

        Sets `failbit` if the read position is at the end of `s`. Modifies
        `completion` only after reading a whole completion.

        \throws std::runtime_error if the text from the read position is not
                a completion, or gives a negative duration
     */
    friend std::istream& operator>>(std::istream& s, test_execution_completion& completion);
  };

  [[nodiscard]]
  std::string to_string(const test_execution_completion& completion);

  /** \brief The start of a test's execution, and its completion if the test
             has finished.

      `operator>>` reads back what `operator<<` writes, provided that the
      start lies in the years 0 to 32767 and that no duration is negative.
   */
  struct test_execution_record
  {
    using start_type = std::chrono::sys_time<std::chrono::milliseconds>;

    start_type start{};
    std::optional<test_execution_completion> completion{};

    [[nodiscard]]
    friend bool operator==(const test_execution_record&, const test_execution_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const test_execution_record& record);

    /** \brief Reads a record which runs from the read position of `s` to
               the end of `s`.

        Once the start is read, any text that follows is read as the
        record's completion, which runs to the end of `s`.

        Sets `failbit` if the read position is at the end of `s`. Modifies
        `record` only after reading a whole record.

        \throws std::runtime_error if the text from the read position is not
                a record, or gives a negative duration
     */
    friend std::istream& operator>>(std::istream& s, test_execution_record& record);
  };

  [[nodiscard]]
  std::string to_string(const test_execution_record& record);

  /** \brief Reads the record of a test's execution held by `file`.

      \throws std::runtime_error if `file` cannot be opened, or does not hold
              a record
   */
  [[nodiscard]]
  test_execution_record read_test_execution_record(const std::filesystem::path& file);

  /** \brief The start of a run of tests.

      A run of tests is a call to `test_runner::execute` which has tests to
      run. Under `locate-instabilities`, one run covers every repetition,
      including the repetitions made in separate processes.

      `operator>>` reads back what `operator<<` writes, provided that the
      start lies in the years 0 to 32767.
   */
  struct run_stamp
  {
    using start_type = test_execution_record::start_type;

    start_type start{};

    [[nodiscard]]
    friend bool operator==(const run_stamp&, const run_stamp&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const run_stamp& stamp);

    /** \brief Reads a stamp from the line at the read position of `s`.

        Sets `failbit` if the read position is at the end of `s`. Modifies
        `stamp` only after reading a whole stamp. Any line after the stamp's
        is left unread.

        \throws std::runtime_error if the line is not a stamp
     */
    friend std::istream& operator>>(std::istream& s, run_stamp& stamp);
  };

  [[nodiscard]]
  std::string to_string(const run_stamp& stamp);

  /** \brief Reads the stamp held by `file`.

      \throws std::runtime_error if `file` cannot be opened, or does not hold
              a stamp
   */
  [[nodiscard]]
  run_stamp read_run_stamp(const std::filesystem::path& file);
}
