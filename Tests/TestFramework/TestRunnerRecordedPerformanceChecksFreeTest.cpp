////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerRecordedPerformanceChecksFreeTest.hpp"
#include "FakeProjectTestingUtilities.hpp"
#include "PerformanceTestingUtilities.hpp"
#include "TestRunnerTestingUtilities.hpp"

#include "sequoia/TestFramework/TestExecutionRecord.hpp"

#include <chrono>
#include <stdexcept>
#include <vector>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    /** Copies the summary `runner` wrote for `Test` into `directory`. */
    template<concrete_test Test>
    void copy_summary(const test_runner& runner, const fs::path& directory)
    {
      const test_summary_path summary{Test::source_file(), test_name<Test>(), runner.proj_paths(), null_discriminator};
      fs::copy_file(summary.file_path(), directory / summary.file_path().filename());
    }

    constexpr std::size_t too_few_trials{relative_performance_minimum_trials_floor - 1};

    constexpr std::chrono::microseconds unit_duration{250};

    /** Makes five performance checks. A check stops at its first passing
        attempt, so the tasks' durations set how many attempts each makes:
        -# Two: the slow task is as fast as the fast one for the first
           attempt's trials, and eight times slower after them.
        -# None: the check is refused before its first attempt.
        -# None recorded: the fast task throws during the first attempt.
        -# One: a speed-up of 8 passes at once.
        -# Three, the most allowed: a speed-up of 1 never passes.
     */
    class attempt_counting_test final : public scripted_clock_test
    {
    public:
      using scripted_clock_test::scripted_clock_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};
        scripted_clock clock{};

        check_relative_performance("A speed-up of 1, then of 8",
                                   counted_scripted_task{clock, unit_duration},
                                   counted_scripted_task{clock,
                                                         unit_duration,
                                                         trials_per_attempt_for_minimum_10[0],
                                                         8 * unit_duration},
                                   parameters,
                                   clock);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters, &clock]() {
            return check_relative_performance("Too few trials",
                                              counted_scripted_task{clock, unit_duration},
                                              counted_scripted_task{clock, 8 * unit_duration},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}},
                                              clock);
          }
        );

        check_exception_thrown<std::runtime_error>(
          "A task which throws ends the check",
          [this, &parameters, &clock]() {
            return check_relative_performance("A throwing task",
                                              []() { throw std::runtime_error{"Fast task failure"}; },
                                              counted_scripted_task{clock, 8 * unit_duration},
                                              parameters,
                                              clock);
          }
        );

        check_relative_performance("A speed-up of 8",
                                   counted_scripted_task{clock, unit_duration},
                                   counted_scripted_task{clock, 8 * unit_duration},
                                   parameters,
                                   clock);

        check_relative_performance("A speed-up of 1",
                                   counted_scripted_task{clock, unit_duration},
                                   counted_scripted_task{clock, unit_duration},
                                   parameters,
                                   clock);
      }
    };

    /** Makes a performance check, and two more, each within a check of an
        exception:
        -# A speed-up of 8 passes at once.
        -# A check of too few trials is refused, which is the exception
           expected.
        -# A speed-up of 0.25 never passes; a check, then the exception
           expected, follow.
     */
    class performance_checks_within_other_checks_test final : public scripted_clock_test
    {
    public:
      using scripted_clock_test::scripted_clock_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};
        scripted_clock clock{};

        check_relative_performance("A speed-up of 8",
                                   counted_scripted_task{clock, unit_duration},
                                   counted_scripted_task{clock, 8 * unit_duration},
                                   parameters,
                                   clock);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters, &clock]() {
            return check_relative_performance("Too few trials",
                                              counted_scripted_task{clock, unit_duration},
                                              counted_scripted_task{clock, 8 * unit_duration},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}},
                                              clock);
          }
        );

        check_exception_thrown<std::runtime_error>(
          "A nested performance check's failure is the enclosing check's failure, and a check may follow it",
          [this, &parameters, &clock]() {
            check_relative_performance("A speed-up of 0.25",
                                       counted_scripted_task{clock, 4 * unit_duration},
                                       counted_scripted_task{clock, unit_duration},
                                       parameters,
                                       clock);
            check("A check after the performance check", true);
            throw std::runtime_error{"After the performance check"};
          }
        );
      }
    };

    /** In false-positive mode, makes a performance check, and another within
        a check of an exception:
        -# A speed-up of 8 passes at once.
        -# A check of too few trials is refused, which is the exception
           expected.
     */
    class performance_checks_within_other_checks_false_positive_test final : public scripted_clock_false_positive_test
    {
    public:
      using scripted_clock_false_positive_test::scripted_clock_false_positive_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};
        scripted_clock clock{};

        check_relative_performance("A speed-up of 8",
                                   counted_scripted_task{clock, unit_duration},
                                   counted_scripted_task{clock, 8 * unit_duration},
                                   parameters,
                                   clock);

        check_exception_thrown<std::invalid_argument>(
          "Too few trials are refused",
          [this, &parameters, &clock]() {
            return check_relative_performance("Too few trials",
                                              counted_scripted_task{clock, unit_duration},
                                              counted_scripted_task{clock, 8 * unit_duration},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}},
                                              clock);
          }
        );
      }
    };

    /** In false-negative mode, makes a performance check, and another within
        a check of an exception, each meant to fail:
        -# A speed-up of 1 fails its first attempt.
        -# A check of too few trials is refused, which is not the exception
           expected.
     */
    class performance_checks_within_other_checks_false_negative_test final : public scripted_clock_false_negative_test
    {
    public:
      using scripted_clock_false_negative_test::scripted_clock_false_negative_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        const relative_performance_parameters parameters{.prediction{.lower{2}, .upper{40}}};
        scripted_clock clock{};

        check_relative_performance("A speed-up of 1",
                                   counted_scripted_task{clock, unit_duration},
                                   counted_scripted_task{clock, unit_duration},
                                   parameters,
                                   clock);

        check_exception_thrown<std::runtime_error>(
          "Too few trials are refused, but not with the exception expected",
          [this, &parameters, &clock]() {
            return check_relative_performance("Too few trials",
                                              counted_scripted_task{clock, unit_duration},
                                              counted_scripted_task{clock, 8 * unit_duration},
                                              {.prediction{parameters.prediction}, .minimum_trials{too_few_trials}},
                                              clock);
          }
        );
      }
    };

    /** A scripted-clock test which makes no performance check. */
    class scripted_clock_test_without_performance_checks final : public scripted_clock_test
    {
    public:
      using scripted_clock_test::scripted_clock_test;

      [[nodiscard]]
      static fs::path source_file()
      {
        return std::source_location::current().file_name();
      }

      void run_tests()
      {
        check(equality, "Not a performance check", 42, 42);
      }
    };
  }

  [[nodiscard]]
  fs::path test_runner_recorded_performance_checks_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_runner_recorded_performance_checks_free_test::run_tests()
  {
    test_recording_of_performance_check_attempts();
    test_summary_of_performance_checks_within_other_checks();
  }

  /** `attempt_counting_test`'s checks make 2, 1 and 3 attempts: distinct
      numbers, in neither sorted order nor its reverse, and not the checks'
      numbers of trials. Its other two checks end by throwing, one before its
      first attempt and one during it, so the record has no entry for either.
      `scripted_clock_test_without_performance_checks` is the control: a test
      on the same base whose record gives no attempts.
   */
  void test_runner_recorded_performance_checks_free_test::test_recording_of_performance_check_attempts()
  {
    std::stringstream outputStream{};
    commandline_arguments args{fake_project_commandline(auxiliary_materials())};
    auto runner{make_fake_project_runner(args, outputStream)};
    runner.register_test<attempt_counting_test>();
    runner.register_test<scripted_clock_test_without_performance_checks>();
    check(equality, "Performance check records return code", runner.execute(), return_code::soft_failures);

    check(equality,
          "The record of a test gives the number of attempts each performance check made, in the order of the checks",
          get_recorded_completion<attempt_counting_test>(runner).performance_check_attempts,
          std::vector<std::size_t>{2, 1, 3});

    check(equality,
          "The record of a test which makes no performance check gives no attempts",
          get_recorded_completion<scripted_clock_test_without_performance_checks>(runner).performance_check_attempts,
          std::vector<std::size_t>{});
  }

  void test_runner_recorded_performance_checks_free_test::test_summary_of_performance_checks_within_other_checks()
  {
    using standard_test       = performance_checks_within_other_checks_test;
    using false_positive_test = performance_checks_within_other_checks_false_positive_test;
    using false_negative_test = performance_checks_within_other_checks_false_negative_test;

    std::stringstream outputStream{};
    commandline_arguments args{fake_project_commandline(auxiliary_materials())};
    auto runner{make_fake_project_runner(args, outputStream)};
    runner.register_test<standard_test>();
    runner.register_test<false_positive_test>();
    runner.register_test<false_negative_test>();
    check(equality,
          "Performance checks within other checks return code",
          runner.execute(),
          return_code::soft_failures);

    const auto outputDir{working_materials() /= "PerformanceChecksWithinOtherChecks"};
    fs::create_directory(outputDir);

    copy_summary<standard_test>(runner, outputDir);
    copy_summary<false_positive_test>(runner, outputDir);
    copy_summary<false_negative_test>(runner, outputDir);

    check(equivalence,
          "A performance check within another check is counted as part of the other",
          outputDir,
          predictive_materials() /= "PerformanceChecksWithinOtherChecks");
  }
}
