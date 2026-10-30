////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

export module sequoia.test_framework:TestExecutionRecord;

import std;

/** \file
    \brief The record of a test's execution and the stamp of a
           \ref sequoia::testing::run_stamp "run of tests", together with
           the functions which write and read them.
 */

export namespace sequoia::testing
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

    /** \brief Reads a completion from the read position of `s`, and leaves
               any text after it unread.

        Sets `failbit` if the read position is at the end of `s`. Modifies
        `completion` only after reading a whole completion.

        \throws std::runtime_error if the text from the read position does
                not begin with a completion, or gives a negative duration
     */
    friend std::istream& operator>>(std::istream& s, test_execution_completion& completion);
  };

  [[nodiscard]]
  std::string to_string(const test_execution_completion& completion);

  /** \brief The start time and completion of a test's execution.

      `completion` is `std::nullopt` if the execution has not finished.

      `operator>>` reads back what `operator<<` writes, provided that the
      start time lies in the years 0 to 32767 and that no duration is
      negative.
   */
  struct test_execution_record
  {
    using start_time_type = std::chrono::sys_time<std::chrono::milliseconds>;

    start_time_type start_time{};
    std::optional<test_execution_completion> completion{};

    [[nodiscard]]
    friend bool operator==(const test_execution_record&, const test_execution_record&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const test_execution_record& record);

    /** \brief Reads a record from the read position of `s`, and leaves any
               text after it unread.

        Sets `failbit` if the read position is at the end of `s`. Modifies
        `record` only after reading a whole record.

        \throws std::runtime_error if the text from the read position does
                not begin with a record, or gives a negative duration
     */
    friend std::istream& operator>>(std::istream& s, test_execution_record& record);
  };

  [[nodiscard]]
  std::string to_string(const test_execution_record& record);

  /** \brief Reads every `test_execution_record` in `file`, in the order in
             which `file` holds them.

      \returns The records; none if `file` is empty.

      \throws std::runtime_error if `file` cannot be opened, or holds
              anything other than a sequence of records
   */
  [[nodiscard]]
  std::vector<test_execution_record> read_test_execution_records(const std::filesystem::path& file);

  /** \brief The start time of a run of tests.

      A run of tests begins when a `test_runner` is constructed with
      arguments which ask it to run tests, whether or not any actually run
      after filtering. Under `locate-instabilities`, one run covers every
      repetition. A runner which makes a repetition in a separate process
      begins no run of its own: the repetition belongs to the run of the
      runner which launched the process.

      A `test_execution_record` is from an earlier run of tests if the
      record's start time precedes the stamp's.

      `operator>>` reads back what `operator<<` writes, provided that the
      start time lies in the years 0 to 32767.
   */
  struct run_stamp
  {
    using start_time_type = test_execution_record::start_time_type;

    start_time_type start_time{};

    [[nodiscard]]
    friend bool operator==(const run_stamp&, const run_stamp&) noexcept = default;

    friend std::ostream& operator<<(std::ostream& s, const run_stamp& stamp);

    /** \brief Reads a stamp from the read position of `s`, and leaves any
               text after it unread.

        Sets `failbit` if the read position is at the end of `s`. Modifies
        `stamp` only after reading a whole stamp.

        \throws std::runtime_error if the text from the read position does
                not begin with a stamp
     */
    friend std::istream& operator>>(std::istream& s, run_stamp& stamp);
  };

  [[nodiscard]]
  std::string to_string(const run_stamp& stamp);

  /** \brief Reads the stamp held by `file`.

      \throws std::runtime_error if `file` cannot be opened, or holds
              anything other than one stamp
   */
  [[nodiscard]]
  run_stamp read_run_stamp(const std::filesystem::path& file);
}
