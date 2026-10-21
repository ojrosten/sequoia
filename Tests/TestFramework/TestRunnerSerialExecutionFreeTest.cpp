////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "TestRunnerSerialExecutionFreeTest.hpp"
#include "FakeProjectTestingUtilities.hpp"
#include "TestRunnerConcurrencyTestingUtilities.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <ranges>
#include <stdexcept>

namespace sequoia::testing
{
  namespace fs = std::filesystem;

  namespace
  {
    struct serial_family {};

    using serial_family_base = test_which_sleeps_base<serial_family>;

    class test_which_sleeps_0 final : public serial_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(0); }
    };

    class test_which_sleeps_1 final : public serial_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(1); }
    };

    class test_which_sleeps_2 final : public serial_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(2); }
    };

    struct active_family {};

    using active_family_base = test_which_sleeps_base<active_family>;

    class active_test_which_sleeps final : public active_family_base
    {
    public:
      using test_which_sleeps_base::test_which_sleeps_base;

      void run_tests() { sleep_and_check(0); }
    };

    struct idle_family {};

    using idle_family_base = test_which_sleeps_base<idle_family>;

    template<class Family, class... TestsWhichSleep>
    inline constexpr bool make_runner_of_tests_which_sleep_accepts_v{
      requires(std::vector<std::string> arguments, std::stringstream& outputStream) {
        make_runner_of_tests_which_sleep<Family, TestsWhichSleep...>(arguments,
                                                                     outputStream,
                                                                     sleep_duration_for_overlap_checks);
      }
    };
  }

  [[nodiscard]]
  fs::path test_runner_serial_execution_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void test_runner_serial_execution_free_test::check_output(std::string_view description,
                                                            std::string_view dirName,
                                                            const std::stringstream& output)
  {
    const auto outputDir{working_materials() /= dirName};
    fs::create_directory(outputDir);
    write_to_file(outputDir / "io.txt", output.view(), std::ios_base::out);
    check(equivalence, description, outputDir, predictive_materials() /= dirName);
  }

  void test_runner_serial_execution_free_test::run_tests()
  {
    test_constraints_on_make_runner_of_tests_which_sleep();
    test_reversed_and_empty_steady_clock_intervals();
    test_index_beyond_registered_tests();
    test_separation_of_families_of_tests_which_sleep();
    test_serial_execution();
  }

  void test_runner_serial_execution_free_test::test_constraints_on_make_runner_of_tests_which_sleep()
  {
    STATIC_CHECK( make_runner_of_tests_which_sleep_accepts_v<serial_family,
                                                             test_which_sleeps_0,
                                                             test_which_sleeps_1,
                                                             test_which_sleeps_2>);
    STATIC_CHECK(!make_runner_of_tests_which_sleep_accepts_v<serial_family>);
    STATIC_CHECK(!make_runner_of_tests_which_sleep_accepts_v<serial_family, serial_family_base>);
    STATIC_CHECK(!make_runner_of_tests_which_sleep_accepts_v<serial_family,
                                                             test_which_sleeps_0,
                                                             active_test_which_sleeps>);
  }

  void test_runner_serial_execution_free_test::test_reversed_and_empty_steady_clock_intervals()
  {
    const auto now{std::chrono::steady_clock::now()};

    check_exception_thrown<std::invalid_argument>(
      "A steady clock interval which ends before it starts is refused",
      [now]() { return steady_clock_interval{now, now - std::chrono::steady_clock::duration{1}}; }
    );

    check(equality,
          "An empty steady clock interval contains no time point",
          peak_overlap(std::array{steady_clock_interval{now, now}}),
          std::size_t{});
  }

  void test_runner_serial_execution_free_test::test_index_beyond_registered_tests()
  {
    std::stringstream outputStream{};
    auto runner{make_runner_of_tests_which_sleep<serial_family, test_which_sleeps_1>(
                  fake_project_commandline(auxiliary_materials()),
                  outputStream,
                  std::chrono::milliseconds{}
                )};

    check(equality,
          "A test which sleeps fails critically if the index it passes is not less than the number of tests registered",
          runner.execute(),
          return_code::critical_failures);
  }

  /** If the two families shared their state, the idle family's sleep
      duration would replace the active family's, and the active test would
      write the idle family's sleep interval.
   */
  void test_runner_serial_execution_free_test::test_separation_of_families_of_tests_which_sleep()
  {
    std::stringstream activeOutput{};
    auto activeRunner{make_runner_of_tests_which_sleep<active_family, active_test_which_sleeps>(
                        fake_project_commandline(auxiliary_materials()),
                        activeOutput,
                        std::chrono::milliseconds{}
                      )};

    constexpr std::chrono::seconds idleFamilySleepDuration{1};
    idle_family_base::prepare_run(1, idleFamilySleepDuration);

    check(equality, "Active family's return code", activeRunner.execute(), return_code::success);

    check("A family's run leaves another family's sleep intervals as they were",
          std::ranges::equal(idle_family_base::sleep_intervals(), std::array{steady_clock_interval{}}));

    auto durationOf{[](const steady_clock_interval& interval) { return interval.end_time() - interval.start_time(); }};
    const auto longestSleep{
      std::ranges::fold_left(active_family_base::sleep_intervals() | std::views::transform(durationOf),
                             std::chrono::steady_clock::duration{},
                             std::ranges::max)
    };

    check(std::ranges::less{},
          "A family's tests sleep for their family's duration",
          longestSleep,
          std::chrono::steady_clock::duration{idleFamilySleepDuration});
  }

  void test_runner_serial_execution_free_test::test_serial_execution()
  {
    std::stringstream outputStream{};
    auto runner{make_runner_of_tests_which_sleep<serial_family,
                                                 test_which_sleeps_0,
                                                 test_which_sleeps_1,
                                                 test_which_sleeps_2>(
                  fake_project_commandline(auxiliary_materials(), "--serial"),
                  outputStream,
                  sleep_duration_for_overlap_checks
                )};
    check(equality, "Serial execution return code", runner.execute(), return_code::success);

    scratch_report_run("serial", serial_family_base::sleep_intervals());
    check_output(report({"Serial Output"}), "Serial Output", outputStream);
    check(equality,
          "No two tests' sleeps overlap in a serial run",
          peak_overlap(serial_family_base::sleep_intervals()),
          std::size_t{1});
  }
}
