////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerReportedDurationsFreeTest.hpp"
#include "TestRunnerTestingUtilities.hpp"

#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/PlatformSpecific/Helpers.hpp"
#include "sequoia/Streaming/Streaming.hpp"
#include "sequoia/TextProcessing/Numbers.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <format>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    using fractional_milliseconds = std::chrono::duration<double, std::milli>;

    /** Reads `printedDuration`, a duration as printed by
        `stringify_duration`.

        Tries the number in `printedDuration` as seconds, milliseconds,
        microseconds and nanoseconds, and returns the first duration which
        `stringify_duration` prints as `printedDuration`.

        \throws std::runtime_error if none does
     */
    [[nodiscard]]
    fractional_milliseconds read_printed_duration(std::string_view printedDuration)
    {
      constexpr std::array<fractional_milliseconds, 4> units{
        std::chrono::seconds{1}, std::chrono::milliseconds{1}, std::chrono::microseconds{1}, std::chrono::nanoseconds{1}
      };

      constexpr fractional_milliseconds representableLimit{log_summary::duration::max()};

      auto reprintsAsPrinted{
        [printedDuration, representableLimit](fractional_milliseconds candidate) {
          if(!(std::chrono::abs(candidate) < representableLimit))
            return false;

          const auto [time, unit]{stringify_duration(std::chrono::round<log_summary::duration>(candidate))};
          return std::format("{}{}", time, unit) == printedDuration;
        }
      };

      if(const auto optNumber{extract_numbers_from<double, 1>(printedDuration)})
      {
        const auto [count]{*optNumber};
        auto candidateIn{[count](fractional_milliseconds unit) { return count * unit; }};
        auto candidates{units | std::views::transform(candidateIn)};
        if(const auto found{std::ranges::find_if(candidates, reprintsAsPrinted)}; found != candidates.end())
          return *found;
      }

      throw std::runtime_error{std::format("Unable to read the duration {}", printedDuration)};
    }

    /** Returns the duration labelled `label` in the grand totals within
        `printedOutput`.
     */
    [[nodiscard]]
    fractional_milliseconds get_grand_total(std::string_view printedOutput, std::string_view label)
    {
      const auto pattern{std::format("[{}: ", label)};
      if(const auto pos{printedOutput.find(pattern, printedOutput.find("Grand Totals"))}; pos != std::string_view::npos)
      {
        const auto start{pos + pattern.size()};
        if(const auto end{printedOutput.find(']', start)}; end != std::string_view::npos)
          return read_printed_duration(printedOutput.substr(start, end - start));
      }

      throw std::runtime_error{std::format("Unable to extract the {} from the printed output", label)};
    }

    [[nodiscard]]
    fractional_milliseconds in_milliseconds(std::chrono::microseconds duration)
    {
      return duration;
    }

    constexpr fractional_milliseconds execution_sleep{20}, summarization_sleep{60};

    /** The runner prints a duration to three significant figures, so it
        prints a duration of less than a second to within half a millisecond.
     */
    constexpr fractional_milliseconds printed_duration_tolerance{1};

    /** Sleeps while its tests run, and sleeps again while the runner summarizes it. The runner summarizes a test after
        its tests have run, so the first sleep falls in the test's execution duration and the second in the runner's
        overhead.
     */
    class slow_to_summarize_test_base : public free_test
    {
    public:
      using free_test::free_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        precise_sleep_for(std::chrono::duration_cast<std::chrono::nanoseconds>(execution_sleep));
        check("Slept", true);
      }

      [[nodiscard]]
      log_summary summarize(duration delta) const
      {
        precise_sleep_for(std::chrono::duration_cast<std::chrono::nanoseconds>(summarization_sleep));
        return free_test::summarize(delta);
      }
    protected:
      ~slow_to_summarize_test_base() = default;

      slow_to_summarize_test_base(slow_to_summarize_test_base&&)            noexcept = default;
      slow_to_summarize_test_base& operator=(slow_to_summarize_test_base&&) noexcept = default;
    };

    class slow_to_summarize_test_0 final : public slow_to_summarize_test_base
    {
    public:
      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    class slow_to_summarize_test_1 final : public slow_to_summarize_test_base
    {
    public:
      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    class slow_to_summarize_test_2 final : public slow_to_summarize_test_base
    {
    public:
      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };

    class unparallelizable_slow_to_summarize_test final : public slow_to_summarize_test_base
    {
    public:
      using parallelizable_type = std::false_type;

      using slow_to_summarize_test_base::slow_to_summarize_test_base;
    };
  }

  [[nodiscard]]
  fs::path test_runner_reported_durations_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  [[nodiscard]]
  fs::path test_runner_reported_durations_free_test::minimal_fake_path() const
  {
    return auxiliary_materials() /= "FakeProject/build/CMade/FakeExe.txt";
  }

  void test_runner_reported_durations_free_test::check_output(std::string_view description,
                                                              std::string_view dirName,
                                                              const std::stringstream& output)
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);
    write_to_file(outputDir / "io.txt", output.view(), std::ios_base::out);
    check(equivalence, description, outputDir, predictive_materials() /= dirName);
  }

  void test_runner_reported_durations_free_test::run_tests()
  {
    test_runner_overhead_reported_apart();
    test_execution_duration_of_busiest_thread();
  }

  /** One test sleeps while its tests run, and again while the runner summarizes it. Its execution duration must
      lie between the first sleep and the sum of the two sleeps. The sum is what the execution duration would be if
      it included the runner's overhead. The printed output and the test's execution record must both say so.

      The second sleep falls in the test's runner overhead, so the run's scheduling overhead must exclude it.
   */
  void test_runner_reported_durations_free_test::test_runner_overhead_reported_apart()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{minimal_fake_path().generic_string(), "--serial"}};
    auto runner{make_fake_project_runner(args, outputStream)};
    runner.register_test<slow_to_summarize_test_0>();
    check(equality, "Runner overhead return code", runner.execute(), return_code::success);

    check_output(report({"Runner Overhead Output"}), "RunnerOverheadOutput", outputStream);
    check(within_tolerance{summarization_sleep / 2},
          "The printed execution duration excludes the sleep while summarizing",
          get_grand_total(outputStream.view(), "Execution Time"),
          execution_sleep + summarization_sleep / 2);

    check(std::ranges::greater_equal{},
          "The printed runner overhead includes the sleep while summarizing",
          get_grand_total(outputStream.view(), "Runner Overhead"),
          summarization_sleep);

    check(std::ranges::less{},
          "The printed scheduling overhead excludes the sleep while summarizing",
          get_grand_total(outputStream.view(), "Scheduling Overhead"),
          summarization_sleep);

    check(within_tolerance{summarization_sleep / 2},
          "The recorded execution duration excludes the sleep while summarizing",
          in_milliseconds(get_recorded_completion<slow_to_summarize_test_0>(runner).execution_duration),
          execution_sleep + summarization_sleep / 2);

    check(std::ranges::greater_equal{},
          "The recorded runner overhead includes the sleep while summarizing",
          in_milliseconds(get_recorded_completion<slow_to_summarize_test_0>(runner).runner_overhead),
          summarization_sleep);
  }

  /** Four tests, each sleeping while its tests run and again while the runner summarizes it. One is not
      parallelizable, so it runs first, alone. The other three then run over a pool of two threads, so one thread runs
      two of them. The run's execution duration must therefore be the first test's recorded execution duration plus
      those of two others. Each rival misses by at least a test's execution duration, far more than the printed
      duration's rounding:
      -# summing every test's execution duration adds the third of the others;
      -# leaving out the first test subtracts its execution duration;
      -# taking the longest test rather than the busiest thread subtracts the execution duration of one of a pair;
      -# a wall clock adds the runner's overhead.

      The summarizing sleeps of the tests on the critical path fall in the run's runner overhead, so none falls in its
      scheduling overhead.
   */
  void test_runner_reported_durations_free_test::test_execution_duration_of_busiest_thread()
  {
    std::stringstream outputStream{};
    commandline_arguments args{{minimal_fake_path().generic_string(), "--thread-pool", "2"}};
    auto runner{make_fake_project_runner(args, outputStream)};
    runner.register_test<unparallelizable_slow_to_summarize_test>();
    runner.register_test<slow_to_summarize_test_0>();
    runner.register_test<slow_to_summarize_test_1>();
    runner.register_test<slow_to_summarize_test_2>();
    check(equality, "Busiest thread return code", runner.execute(), return_code::success);

    check_output(report({"Busiest Thread Output"}), "BusiestThreadOutput", outputStream);

    const auto firstDuration{
      in_milliseconds(get_recorded_completion<unparallelizable_slow_to_summarize_test>(runner).execution_duration)
    };
    const std::array othersDurations{
      in_milliseconds(get_recorded_completion<slow_to_summarize_test_0>(runner).execution_duration),
      in_milliseconds(get_recorded_completion<slow_to_summarize_test_1>(runner).execution_duration),
      in_milliseconds(get_recorded_completion<slow_to_summarize_test_2>(runner).execution_duration)
    };

    const std::array pairedDurations{
      othersDurations[0] + othersDurations[1],
      othersDurations[0] + othersDurations[2],
      othersDurations[1] + othersDurations[2]
    };

    const auto executionDuration{get_grand_total(outputStream.view(), "Execution Time")};
    auto distanceFromExecutionDuration{
      [executionDuration, firstDuration](fractional_milliseconds paired){
        return std::chrono::abs(executionDuration - (firstDuration + paired));
      }
    };

    const auto nearestPairedDuration{
      std::ranges::min(pairedDurations, std::ranges::less{}, distanceFromExecutionDuration)
    };
    check(within_tolerance{printed_duration_tolerance},
          "The execution duration is the unparallelizable test's plus those of the two others one thread ran",
          executionDuration,
          firstDuration + nearestPairedDuration);

    check(std::ranges::greater_equal{},
          "The runner overhead includes the summarizing sleeps of the unparallelizable test and the busiest thread",
          get_grand_total(outputStream.view(), "Runner Overhead"),
          3 * summarization_sleep);

    check(std::ranges::less{},
          "The scheduling overhead excludes every summarizing sleep",
          get_grand_total(outputStream.view(), "Scheduling Overhead"),
          summarization_sleep);
  }
}
