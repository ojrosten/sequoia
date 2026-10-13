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
#include <ranges>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    test_execution_record unfinished_record()
    {
      using namespace std::chrono;
      return {.start_time{sys_days{year{2026} / 10 / 13} + 9h + 15min + 4s + 123ms}};
    }

    [[nodiscard]]
    test_execution_record finished_record()
    {
      using namespace std::chrono_literals;
      return {
        .start_time{unfinished_record().start_time},
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
    test_what_follows();
    test_written_format();
    test_refusals();
    test_reading_files();
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
      firstStart{.start_time{sys_days{year{0} / 1 / 1}}},
      lastStart {.start_time{sys_days{year{32767} / 12 / 31} + days{1} - milliseconds{1}}};

    check(equality,
          "A record with the earliest start time the format holds round-trips through operator<< and operator>>",
          readBackFromWritten(firstStart),
          std::optional{firstStart});

    check(equality,
          "A record with the latest start time the format holds round-trips through operator<< and operator>>",
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

  void test_execution_record_regular_test::test_what_follows()
  {
    auto readFirstOfTwo{
      [](const test_execution_record& first) -> std::optional<std::pair<test_execution_record, test_execution_record>> {
        std::stringstream s{to_string(first) + to_string(finished_record())};

        test_execution_record firstRead{}, secondRead{};
        if(s >> firstRead >> secondRead)
          return std::pair{firstRead, secondRead};

        return std::nullopt;
      }
    };

    check(equality,
          "A record with no completion, then a finished record, read back one after the other",
          readFirstOfTwo(unfinished_record()),
          std::optional{std::pair{unfinished_record(), finished_record()}});

    check(equality,
          "Two finished records read back one after the other",
          readFirstOfTwo(finished_record()),
          std::optional{std::pair{finished_record(), finished_record()}});
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

    check_exception_thrown<std::runtime_error>(
      "A first line which does not give the start time",
      readRecord("begun 2026-10-13T09:15:04.123Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start time in a layout other than the one operator<< writes",
      readRecord("started 2026-10-13 09:15:04.123Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start time with a month out of range",
      readRecord("started 2026-13-13T09:15:04.123Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start time without its milliseconds",
      readRecord("started 2026-10-13T09:15:04Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A start time with an hour out of range",
      readRecord("started 2026-10-13T2147483000:00:00.000Z\n"));

    check_exception_thrown<std::runtime_error>(
      "A record which does not say whether the test finished",
      readRecord("started 2026-10-13T09:15:04.123Z\nexecution duration 1500us\n"));

    check_exception_thrown<std::runtime_error>(
      "A blank line where a record says whether the test finished",
      readRecord("started 2026-10-13T09:15:04.123Z\n\n"));

    check_exception_thrown<std::runtime_error>(
      "A record which says the test finished, but has no completion",
      readRecord("started 2026-10-13T09:15:04.123Z\nfinished yes\n"));

    check_exception_thrown<std::runtime_error>(
      "A record whose completion is refused",
      readRecord("started 2026-10-13T09:15:04.123Z\nfinished yes\nexecution duration 2ms\n"));

    test_execution_record readInto{finished_record()};
    try
    {
      std::stringstream s{"started 2026-10-14T09:15:04.123Z\nfinished yes\nexecution duration 2ms\n"};
      s >> readInto;
    }
    catch(const std::runtime_error&)
    {
    }

    check(equality, "A refused read leaves the record as it was", readInto, finished_record());
  }

  /** `Repetitions.txt` holds the records of two executions of a test, as the runner appends them: each execution's
      record with no completion, then the same record with one.
   */
  void test_execution_record_regular_test::test_reading_files()
  {
    check(equality,
          "An empty file holds no records",
          read_test_execution_records(working_materials() / "Empty.txt"),
          std::vector<test_execution_record>{});

    using namespace std::chrono_literals;
    const auto laterStart{unfinished_record().start_time + 2s};
    const std::vector<test_execution_record> records{
      unfinished_record(),
      finished_record(),
      {.start_time{laterStart}},
      {.start_time{laterStart}, .completion{finished_record().completion}}
    };

    const auto recordsFile{working_materials() / "Repetitions.txt"};
    write_to_file(recordsFile,
                  records | std::views::transform([](const auto& record) { return to_string(record); })
                          | std::views::join
                          | std::ranges::to<std::string>(),
                  std::ios_base::binary);

    check(equivalence,
          "The text of a file of the records of two executions",
          recordsFile,
          predictive_materials() / "Repetitions.txt");

    check(equality,
          "A file of records reads back as every record, in the order written",
          read_test_execution_records(recordsFile),
          records);
  }

  void test_execution_record_regular_test::test_refusals_of_files()
  {
    check_exception_thrown<std::runtime_error>(
      "A file which does not exist",
      [this]() { return read_test_execution_records(working_materials() / "Missing.txt"); });

    check_exception_thrown<std::runtime_error>(
      "A file whose text is not a record",
      [this]() { return read_test_execution_records(working_materials() / "Malformed.txt"); });

    check_exception_thrown<std::runtime_error>(
      "A file with a line after a record which begins no record",
      [this]() { return read_test_execution_records(working_materials() / "Followed.txt"); });
  }
}
