////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestExecutionRecordRegularTest.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <format>
#include <optional>
#include <sstream>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    test_execution_record unfinished_record()
    {
      using namespace std::chrono;
      return {.start{sys_days{year{2026} / 10 / 13} + 9h + 15min + 4s + 123ms}};
    }

    [[nodiscard]]
    test_execution_record finished_record()
    {
      using namespace std::chrono_literals;
      return {
        .start{unfinished_record().start},
        .completion{
          test_execution_completion{
            .execution_duration{1500us},
            .runner_overhead{250us},
            .performance_check_attempts{2, 1, 3}
          }
        }
      };
    }
  }

  [[nodiscard]]
  std::filesystem::path test_execution_record_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_execution_record_regular_test::run_tests()
  {
    test_semantics();
    test_round_trip();
    test_end_of_stream();
    test_written_format();
    test_refusals();
    test_refusals_of_files();
  }

  void test_execution_record_regular_test::test_semantics()
  {
    check_semantics("A default record and a finished record", test_execution_record{}, finished_record());
    check_semantics("Records which differ only in their completion", unfinished_record(), finished_record());
  }

  void test_execution_record_regular_test::test_round_trip()
  {
    auto readBackFromWritten{
      [](const test_execution_record& written) -> std::optional<test_execution_record> {
        std::stringstream s{};
        s << written;

        test_execution_record readBack{.completion{test_execution_completion{.performance_check_attempts{7}}}};
        if(s >> readBack)
          return readBack;

        return std::nullopt;
      }
    };

    check(equality,
          "operator>> overwrites a finished record with a default record which operator<< wrote",
          readBackFromWritten(test_execution_record{}),
          std::optional{test_execution_record{}});

    check(equality,
          "A record with no completion round-trips through operator<< and operator>>",
          readBackFromWritten(unfinished_record()),
          std::optional{unfinished_record()});

    check(equality,
          "A finished record round-trips through operator<< and operator>>",
          readBackFromWritten(finished_record()),
          std::optional{finished_record()});

    const auto finishedWithoutAttempts{
      []() {
        auto record{finished_record()};
        record.completion->performance_check_attempts.clear();
        return record;
      }()
    };

    check(equality,
          "A finished record with no performance check attempts round-trips through operator<< and operator>>",
          readBackFromWritten(finishedWithoutAttempts),
          std::optional{finishedWithoutAttempts});

    using namespace std::chrono;
    const test_execution_record
      firstStart{.start{sys_days{year{0} / 1 / 1}}},
      lastStart {.start{sys_days{year{32767} / 12 / 31} + days{1} - milliseconds{1}}};

    check(equality,
          "A record with the earliest start the format holds round-trips through operator<< and operator>>",
          readBackFromWritten(firstStart),
          std::optional{firstStart});

    check(equality,
          "A record with the latest start the format holds round-trips through operator<< and operator>>",
          readBackFromWritten(lastStart),
          std::optional{lastStart});
  }

  void test_execution_record_regular_test::test_end_of_stream()
  {
    std::stringstream s{};
    test_execution_record record{finished_record()};
    s >> record;

    check("A read at the end of a stream fails", s.fail());
    check(equality, "A read at the end of a stream leaves the record as it was", record, finished_record());

    auto readWithoutFinalLineBreak{
      [](const test_execution_record& written) -> std::optional<test_execution_record> {
        const auto text{to_string(written)};
        std::stringstream unterminated{text.substr(0, text.size() - 1)};

        test_execution_record readBack{};
        if(unterminated >> readBack)
          return readBack;

        return std::nullopt;
      }
    };

    check(equality,
          "A record with no completion and no final line break reads as one with the line break",
          readWithoutFinalLineBreak(unfinished_record()),
          std::optional{unfinished_record()});

    check(equality,
          "A finished record with no final line break reads as one with the line break",
          readWithoutFinalLineBreak(finished_record()),
          std::optional{finished_record()});
  }

  void test_execution_record_regular_test::test_written_format()
  {
    write_to_file(working_materials() / "Unfinished.txt", to_string(unfinished_record()), std::ios_base::binary);
    write_to_file(working_materials() / "Finished.txt",   to_string(finished_record()),   std::ios_base::binary);

    check(equivalence,
          "The text of a record with no completion",
          working_materials() / "Unfinished.txt",
          predictive_materials() / "Unfinished.txt");

    check(equivalence,
          "The text of a finished record",
          working_materials() / "Finished.txt",
          predictive_materials() / "Finished.txt");
  }

  void test_execution_record_regular_test::test_refusals()
  {
    auto readRecord{
      [](std::string text) {
        return [text{std::move(text)}]() {
          std::stringstream s{text};
          test_execution_record record{};
          s >> record;
        };
      }
    };

    const std::string startLine{"started 2026-10-13T09:15:04.123Z\n"},
                      timingLines{"execution duration 1500us\nrunner overhead 250us\n"};

    check_exception_thrown<std::runtime_error>(
      "A first line which does not give the start",
      readRecord("begun 2026-10-13T09:15:04.123Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start in a layout other than the one operator<< writes",
      readRecord("started 2026-10-13 09:15:04.123Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start with a month out of range",
      readRecord("started 2026-13-13T09:15:04.123Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start without its milliseconds",
      readRecord("started 2026-10-13T09:15:04Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start with an hour out of range",
      readRecord("started 2026-10-13T2147483000:00:00.000Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A completion which does not begin with the execution duration",
      readRecord(startLine + "runner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "An execution duration in a unit other than microseconds",
      readRecord(startLine + "execution duration 2ms\nrunner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "An execution duration with a leading zero",
      readRecord(startLine + "execution duration 01500us\nrunner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "A negative runner overhead",
      readRecord(startLine + "execution duration 1500us\nrunner overhead -250us\n"));

    check_exception_thrown<std::runtime_error>(
      "A negative runner overhead whose magnitude exceeds the largest signed count",
      readRecord(startLine + "execution duration 1500us\nrunner overhead -9223372036854775808us\n"));

    check_exception_thrown<std::runtime_error>(
      "A completion with no runner overhead",
      readRecord(startLine + "execution duration 1500us\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of attempts which is not a number",
      readRecord(startLine + timingLines + "performance check attempts two\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of attempts with a leading zero",
      readRecord(startLine + timingLines + "performance check attempts 02\n"));

    check_exception_thrown<std::runtime_error>(
      "A line which gives no number of attempts, after one which does",
      readRecord(startLine + timingLines + "performance check attempts 2\nfoo\n"));

    check_exception_thrown<std::runtime_error>(
      "A runner overhead where a number of attempts should be",
      readRecord(startLine + timingLines + "runner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of attempts with a label other than the one operator<< writes",
      readRecord(startLine + timingLines + "check attempts 2\n"));

    test_execution_record readInto{finished_record()};
    try
    {
      std::stringstream s{"started 2026-10-14T09:15:04.123Z\nexecution duration 2ms\n"};
      s >> readInto;
    }
    catch(const std::runtime_error&)
    {
    }

    check(equality, "A refused read leaves the record as it was", readInto, finished_record());
  }

  void test_execution_record_regular_test::test_refusals_of_files()
  {
    check_exception_thrown<std::runtime_error>(
      "A file which does not exist",
      [this]() { return read_test_execution_record(working_materials() / "Missing.txt"); });

    check_exception_thrown<std::runtime_error>(
      "An empty file",
      [this]() { return read_test_execution_record(working_materials() / "Empty.txt"); });

    check_exception_thrown<std::runtime_error>(
      "A file whose text is not a record",
      [this]() { return read_test_execution_record(working_materials() / "Malformed.txt"); });
  }
}
