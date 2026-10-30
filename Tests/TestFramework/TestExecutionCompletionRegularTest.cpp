////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestExecutionCompletionRegularTest.hpp"

import std;

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    test_execution_completion completion()
    {
      using namespace std::chrono_literals;
      return {.execution_duration{1500us}, .runner_overhead{250us}, .performance_check_attempts{2, 1, 3}};
    }

    [[nodiscard]]
    test_execution_completion completion_without_attempts()
    {
      auto withoutAttempts{completion()};
      withoutAttempts.performance_check_attempts.clear();
      return withoutAttempts;
    }
  }

  [[nodiscard]]
  std::filesystem::path test_execution_completion_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_execution_completion_regular_test::run_tests()
  {
    test_semantics();
    test_round_trip();
    test_end_of_stream();
    test_what_follows();
    test_refusals();
  }

  void test_execution_completion_regular_test::test_semantics()
  {
    check_semantics("A default completion and one with attempts", test_execution_completion{}, completion());
  }

  void test_execution_completion_regular_test::test_round_trip()
  {
    auto readBackFromWritten{
      [](const test_execution_completion& written) -> std::optional<test_execution_completion> {
        std::stringstream s{};
        s << written;

        test_execution_completion readBack{completion()};
        if(s >> readBack)
          return readBack;

        return std::nullopt;
      }
    };

    check(equality,
          "operator>> overwrites a completion with a default completion which operator<< wrote",
          readBackFromWritten(test_execution_completion{}),
          std::optional{test_execution_completion{}});

    check(equality,
          "A completion round-trips through operator<< and operator>>",
          readBackFromWritten(completion()),
          std::optional{completion()});

    check(equality,
          "A completion with no performance check attempts round-trips through operator<< and operator>>",
          readBackFromWritten(completion_without_attempts()),
          std::optional{completion_without_attempts()});
  }

  void test_execution_completion_regular_test::test_end_of_stream()
  {
    std::stringstream s{};
    test_execution_completion readInto{completion()};
    s >> readInto;

    check("A read at the end of a stream fails", s.fail());
    check(equality, "A read at the end of a stream leaves the completion as it was", readInto, completion());

    auto readWithoutFinalLineBreak{
      [](const test_execution_completion& written) -> std::optional<test_execution_completion> {
        const auto text{to_string(written)};
        std::stringstream unterminated{text.substr(0, text.size() - 1)};

        test_execution_completion readBack{};
        if(unterminated >> readBack)
          return readBack;

        return std::nullopt;
      }
    };

    check(equality,
          "A completion with no final line break reads as one with the line break",
          readWithoutFinalLineBreak(completion()),
          std::optional{completion()});
  }

  void test_execution_completion_regular_test::test_what_follows()
  {
    std::stringstream followed{to_string(completion()) + "next line\n"};
    test_execution_completion readBack{};
    const bool readSucceeded{static_cast<bool>(followed >> readBack)};
    std::string rest{};
    std::getline(followed, rest);

    check("A read of a completion followed by another line succeeds", readSucceeded);
    check(equality, "A completion followed by another line reads as the completion", readBack, completion());
    check(equality, "operator>> leaves the line after a completion unread", rest, std::string{"next line"});

    std::stringstream followedNoAttempts{to_string(completion_without_attempts()) + "next line\n"};
    test_execution_completion readNoAttempts{completion()};
    followedNoAttempts >> readNoAttempts;
    std::string restNoAttempts{};
    std::getline(followedNoAttempts, restNoAttempts);

    check(equality,
          "A completion with no attempts, followed by another line, reads as the completion",
          readNoAttempts,
          completion_without_attempts());

    check(equality,
          "operator>> leaves the line after a completion with no attempts unread",
          restNoAttempts,
          std::string{"next line"});

    std::stringstream moreThanCounted{
      "execution duration 1500us\nrunner overhead 250us\nentries of performance check attempts 1\n"
      "performance check attempts 2\nperformance check attempts 3\n"
    };
    test_execution_completion counted{};
    moreThanCounted >> counted;
    std::string uncounted{};
    std::getline(moreThanCounted, uncounted);

    check(equality,
          "A completion reads only as many entries of attempts as it gives",
          counted.performance_check_attempts,
          std::vector<std::size_t>{2});

    check(equality,
          "operator>> leaves an entry of attempts beyond the completion's number unread",
          uncounted,
          std::string{"performance check attempts 3"});
  }

  void test_execution_completion_regular_test::test_refusals()
  {
    auto readCompletion{
      [](std::string text) {
        return [text{std::move(text)}]() {
          std::stringstream s{text};
          test_execution_completion readInto{};
          s >> readInto;
        };
      }
    };

    const std::string timingLines{"execution duration 1500us\nrunner overhead 250us\n"};

    auto withEntries{
      [&timingLines](std::string_view entries) {
        return std::format("{}entries of performance check attempts {}\n", timingLines, entries);
      }
    };

    check_exception_thrown<std::runtime_error>(
      "A completion which does not begin with the execution duration",
      readCompletion("runner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "An execution duration in a unit other than microseconds",
      readCompletion("execution duration 2ms\nrunner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "An execution duration with a leading zero",
      readCompletion("execution duration 01500us\nrunner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "A negative runner overhead",
      readCompletion("execution duration 1500us\nrunner overhead -250us\n"));

    check_exception_thrown<std::runtime_error>(
      "A negative runner overhead whose magnitude exceeds the largest signed count",
      readCompletion("execution duration 1500us\nrunner overhead -9223372036854775808us\n"));

    check_exception_thrown<std::runtime_error>(
      "A completion with no runner overhead",
      readCompletion("execution duration 1500us\n"));

    check_exception_thrown<std::runtime_error>(
      "A completion which does not give its number of entries of attempts",
      readCompletion(timingLines + "performance check attempts 2\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of entries of attempts which is not a number",
      readCompletion(withEntries("two")));

    check_exception_thrown<std::runtime_error>(
      "A number of attempts which is not a number",
      readCompletion(withEntries("1") + "performance check attempts two\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of attempts with a leading zero",
      readCompletion(withEntries("1") + "performance check attempts 02\n"));

    check_exception_thrown<std::runtime_error>(
      "A line which gives no number of attempts, after one which does",
      readCompletion(withEntries("2") + "performance check attempts 2\nfoo\n"));

    check_exception_thrown<std::runtime_error>(
      "A runner overhead where a number of attempts should be",
      readCompletion(withEntries("1") + "runner overhead 250us\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of attempts with a label other than the one operator<< writes",
      readCompletion(withEntries("1") + "check attempts 2\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of entries of attempts far beyond the entries which follow",
      readCompletion(withEntries("9223372036854775807") + "performance check attempts 2\n"));

    check_exception_thrown<std::runtime_error>(
      "A number of entries of attempts beyond the largest std::ptrdiff_t",
      readCompletion(withEntries("9223372036854775808") + "performance check attempts 2\n"));

    check_exception_thrown<std::runtime_error>(
      "Fewer entries of attempts than the completion says it has",
      readCompletion(withEntries("2") + "performance check attempts 2\n"));

    test_execution_completion readInto{completion()};
    try
    {
      std::stringstream s{
        "execution duration 9us\nrunner overhead 1us\nentries of performance check attempts 1\n"
        "performance check attempts two\n"
      };
      s >> readInto;
    }
    catch(const std::runtime_error&)
    {
    }

    check(equality, "A refused read leaves the completion as it was", readInto, completion());
  }
}
