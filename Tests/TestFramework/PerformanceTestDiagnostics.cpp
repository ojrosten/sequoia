////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <array>
#include <chrono>
#include <limits>
#include <memory>
#include <utility>

namespace sequoia::testing
{
  namespace
  {
    /** \brief Keeps the calling thread busy until `t` has passed by the
               steady clock.

        A sleep ends on the operating system's timer, and so overruns by an
        amount that varies with the platform and the load, distorting the
        ratios the checks below predict. A spin ends at the first reading of
        the clock past its deadline.
     */
    void spin_for(std::chrono::milliseconds t)
    {
      const auto deadline{std::chrono::steady_clock::now() + t};
      while(std::chrono::steady_clock::now() < deadline) {}
    }

    /** \brief Returns a task which spins for each of `durations` in turn,
               starting again after the last.

        Every copy of the task shares one count of the calls made, so any
        five consecutive calls spin for each duration once.
     */
    [[nodiscard]]
    auto make_cycling_spinner(const std::array<std::chrono::milliseconds, 5>& durations)
    {
      return [durations, calls{std::make_shared<std::size_t>()}]() {
        spin_for(durations[(*calls)++ % durations.size()]);
      };
    }

    struct copyable_task
    {
      void operator()() const {}
    };

    struct move_only_task
    {
      std::unique_ptr<int> resource{};

      void operator()() const {}
    };

    struct rvalue_only_task
    {
      void operator()() && {}
    };

    template<class Task>
    concept profilable = requires(Task task) { profile(std::move(task)); };

    template<class Fast, class Slow>
    inline constexpr bool checkable_tasks_v{
      requires(test_logger<test_mode::standard>& logger,
               Fast fast,
               Slow slow,
               const relative_performance_parameters& parameters) {
        check_relative_performance("", logger, std::move(fast), std::move(slow), parameters);
      }
    };

    template<class Fast, class Slow>
    inline constexpr bool checkable_tasks_by_extender_v{
      requires(performance_extender<test_mode::standard>& extender,
               Fast fast,
               Slow slow,
               const relative_performance_parameters& parameters) {
        extender.check_relative_performance("", std::move(fast), std::move(slow), parameters);
      }
    };
  }

  [[nodiscard]]
  std::filesystem::path performance_false_negative_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
    test_sd_tolerance();
    test_significance_gate();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    constexpr std::chrono::milliseconds deltaT{5};

    check_relative_performance("Performance Test for which fast task is too slow, [1, (2.0, 2.0)",
                               [deltaT]() { spin_for(deltaT); },
                               [deltaT]() { spin_for(deltaT); },
                               {.min_speedup{2.0}, .max_speedup{2.0}, .trials{5}, .num_sds{4}, .max_attempts{3}});

    check_relative_performance("Performance Test for which fast task is too slow [1, (2.0, 3.0)",
                               [deltaT]() { spin_for(deltaT); },
                               [deltaT]() { spin_for(deltaT); },
                               {.min_speedup{2.0}, .max_speedup{3.0}, .trials{5}, .num_sds{4}, .max_attempts{3}});

    check_relative_performance("Performance Test for which fast task is too fast [4, (2.0, 2.5)]",
                               [deltaT]() { spin_for(deltaT); },
                               [deltaT]() { spin_for(4 * deltaT); },
                               {.min_speedup{2.0}, .max_speedup{2.5}, .trials{5}, .num_sds{4}, .max_attempts{3}});
  }

  void performance_false_negative_diagnostics::test_sd_tolerance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Speed-up of 2, below (2.6, 3.0) by more than 1.5 of the slow task's sds",
                               make_cycling_spinner({1ms, 3ms, 5ms, 7ms, 9ms}),
                               make_cycling_spinner({8ms, 9ms, 10ms, 11ms, 12ms}),
                               {.min_speedup{2.6}, .max_speedup{3.0}, .trials{5}, .num_sds{1.5}, .max_attempts{3}});

    check_relative_performance("Speed-up of 2, above (1.2, 1.5) by more than 1.5 of the slow task's sds",
                               make_cycling_spinner({1ms, 3ms, 5ms, 7ms, 9ms}),
                               make_cycling_spinner({8ms, 9ms, 10ms, 11ms, 12ms}),
                               {.min_speedup{1.2}, .max_speedup{1.5}, .trials{5}, .num_sds{1.5}, .max_attempts{3}});

    check_relative_performance("Speed-up of 2, below (4.5, 5.0) by more than 1.5 of the fast task's sds",
                               make_cycling_spinner({3ms, 4ms, 5ms, 6ms, 7ms}),
                               make_cycling_spinner({6ms, 8ms, 10ms, 12ms, 14ms}),
                               {.min_speedup{4.5}, .max_speedup{5.0}, .trials{5}, .num_sds{1.5}, .max_attempts{3}});

    check_relative_performance("Speed-up of 2, above (1.1, 1.3) by more than 1.5 of the fast task's sds",
                               make_cycling_spinner({3ms, 4ms, 5ms, 6ms, 7ms}),
                               make_cycling_spinner({6ms, 8ms, 10ms, 12ms, 14ms}),
                               {.min_speedup{1.1}, .max_speedup{1.3}, .trials{5}, .num_sds{1.5}, .max_attempts{3}});
  }

  void performance_false_negative_diagnostics::test_significance_gate()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Speed-up of 2, within (1.8, 2.1), but the tasks' spreads overlap",
                               make_cycling_spinner({1ms, 3ms, 5ms, 7ms, 9ms}),
                               make_cycling_spinner({2ms, 6ms, 10ms, 14ms, 18ms}),
                               {.min_speedup{1.8}, .max_speedup{2.1}, .trials{5}, .num_sds{4}, .max_attempts{3}});
  }

  [[nodiscard]]
  std::filesystem::path performance_false_positive_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
    test_sd_tolerance();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    constexpr std::chrono::milliseconds deltaT{5};

    check_relative_performance("Performance Test which should pass",
                               [deltaT]() { spin_for(deltaT); },
                               [deltaT]() { spin_for(2 * deltaT); },
                               {.min_speedup{1.8}, .max_speedup{2.1}, .trials{5}, .num_sds{4}, .max_attempts{3}});

    check_relative_performance("Performance Test which should pass",
                               [deltaT]() { spin_for(deltaT); },
                               [deltaT]() { spin_for(4 * deltaT); },
                               {.min_speedup{3.4}, .max_speedup{4.1}, .trials{5}, .num_sds{4}, .max_attempts{3}});
  }

  void performance_false_positive_diagnostics::test_sd_tolerance()
  {
    using namespace std::chrono_literals;

    check_relative_performance("Speed-up of 2, below (2.6, 3.0) by less than 4 of the slow task's sds",
                               make_cycling_spinner({1ms, 3ms, 5ms, 7ms, 9ms}),
                               make_cycling_spinner({8ms, 9ms, 10ms, 11ms, 12ms}),
                               {.min_speedup{2.6}, .max_speedup{3.0}, .trials{5}, .num_sds{4}, .max_attempts{3}});

    check_relative_performance("Speed-up of 2, above (1.2, 1.5) by less than 4 of the slow task's sds",
                               make_cycling_spinner({1ms, 3ms, 5ms, 7ms, 9ms}),
                               make_cycling_spinner({8ms, 9ms, 10ms, 11ms, 12ms}),
                               {.min_speedup{1.2}, .max_speedup{1.5}, .trials{5}, .num_sds{4}, .max_attempts{3}});

    check_relative_performance("Speed-up of 2, below (4.5, 5.0) by less than 4 of the fast task's sds",
                               make_cycling_spinner({3ms, 4ms, 5ms, 6ms, 7ms}),
                               make_cycling_spinner({6ms, 8ms, 10ms, 12ms, 14ms}),
                               {.min_speedup{4.5}, .max_speedup{5.0}, .trials{5}, .num_sds{4}, .max_attempts{3}});

    check_relative_performance("Speed-up of 2, above (1.1, 1.3) by less than 4 of the fast task's sds",
                               make_cycling_spinner({3ms, 4ms, 5ms, 6ms, 7ms}),
                               make_cycling_spinner({6ms, 8ms, 10ms, 12ms, 14ms}),
                               {.min_speedup{1.1}, .max_speedup{1.3}, .trials{5}, .num_sds{4}, .max_attempts{3}});
  }

  [[nodiscard]]
  std::filesystem::path performance_utilities_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_utilities_test::run_tests()
  {
    test_postprocessing();
    test_coarse_sleep();
    test_invalid_arguments();
    test_throwing_task();
    test_task_constraints();
  }

  void performance_utilities_test::test_postprocessing()
  {
    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"foo Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"bar Task duration: 0.00147s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"foo Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "bar Task duration: 0.0015s +- 3 * 0.0019s\n"};
      std::string_view reference{"foo Task duration: 0.00147s +- 3 * 0.0011s\n"
                                 "bar Task duration: 0.00151s +- 3 * 0.0016s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"foo Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "bar Task duration: 0.0015s +- 3 * 0.0019s\n"};
      std::string_view reference{"foo Task duration: 0.00147s +- 3 * 0.0011s\n"
                                 "baz Task duration: 0.00151s +- 3 * 0.0016s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{""};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {""};
      std::string_view reference{"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "bar Task duration: 0.0015s +- 3 * 0.0019s\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s\n"
                                 "bar Task duration: 0.0015s +- 3 * 0.0019s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.9, 4.1)]\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s [3.4; (2.9, 4.1)]\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.9, 4.1)]\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.001s [3.4; (2.9, 4.1)]\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.8, 4.1)]\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s [3.4; (2.9, 4.1)]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.8, 4.1)]\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s [3.4; (2.9, 4)]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 4 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 8.1e-05s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 9.7e-06s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Fast Task duration: 9.9e-05s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 0.000101s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Fast Task duration: 8.1e-05s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 9.7e-06s +- 4 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 0.00145s +- 4 * 0.0014s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.8, 4.1)]\n"};
      std::string_view reference{"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.9, 4.1)]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s [3.4; (2.9, 4.1)]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s [3.4; (2.9, 4.1)]\n"};
      std::string_view reference{"Fast Task duration: 0.00147s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "Line 44\n"};
      std::string_view reference{"Fast Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "Line 45\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"foo Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "bar Task duration: 0.0015s +- 3 * 0.0019s\n"};
      std::string_view reference{"foo Task duration: 0.00145s +- 3 * 0.0014s\n"
                                 "baz Task duration: 0.00151s +- 3 * 0.0016s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Line 40\n"
                                 "Fast Task duration: 0.00627279s +- 4 * 1.04998e-05s\n"
                                 "\n"
                                 "Line 44\n"
                                 "Slow Task duration: 8.1e-05s +- 4 * 8.03116e-06s [1.00256; (2, 2)]\n"};
      std::string_view reference{"Line 40\n"
                                 "Fast Task duration: 0.00602411s +- 4 * 0.000127161s\n"
                                 "\n"
                                 "Line 44\n"
                                 "Slow Task duration: 9.7e-06s +- 4 * 8.69355e-06s [1.04386; (2, 2)]\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.006s +- 4 * 0.0001s\n"};
      std::string_view reference{"Fast Task duration: 0.006ms +- 4 * 0.0001s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 0.006s +- 4 * 0.0001s\n"};
      std::string_view reference{"Fast Task duration: 0.006s +- 4 * 0.0001ms\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Slow Task duration: 0.0063s +- 4 * 8.7e-06s [1.04; (2, 3)]\n"};
      std::string_view reference{"Slow Task duration: 0.0063s +- 4 * 8.7e-06s {1.04; (2, 3)}\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Slow Task duration: 0.0063s +- 4 * 8.7e-06s [1.04; (2, 3)]\n"};
      std::string_view reference{"Slow Task duration: 0.0063s +- 4 * 8.7e-06s (2, 3)\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Slow Task duration: 0.0063s +- 4 * 8.7e-06s [1.04; (2, 3)]\n"};
      std::string_view reference{"Slow Task duration: 0.0063s +- 4 * 8.7e-06s [; (2, 3)]\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast Task duration: 1s +- 4 * 0.1s\n"};
      std::string_view reference{"Fast Task duration: 1s +- 4 * 0.1s FAILED\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Fast 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Fast 0.00147s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      std::string_view latest   {"Task 2 Task duration: 0.00145s +- 3 * 0.0014s\n"};
      std::string_view reference{"Task 2 Task duration: 0.00147s +- 3 * 0.0011s\n"};

      check(equality, "", postprocess(latest, reference), reference);
    }

    {
      std::string_view latest   {"Fast Task duration: s +- 3 * s\n"};
      std::string_view reference{"Fast Task duration: s +- 4 * s\n"};

      check(equality, "", postprocess(latest, reference), latest);
    }

    {
      const std::string latest   {duration_summary("Slow", 9.9e-05,  4, 1e-06)   + speedup_summary(1.2e+06, 2, 3)};
      const std::string reference{duration_summary("Slow", 0.000101, 4, 8.7e-06) + speedup_summary(1.04,    2, 3)};

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      constexpr auto infinity{std::numeric_limits<double>::infinity()};
      constexpr auto nan{std::numeric_limits<double>::quiet_NaN()};

      const std::string latest   {duration_summary("Slow", 0.000999, 4, 1.5e-05) + speedup_summary(infinity, 2, 3)};
      const std::string reference{duration_summary("Slow", 0.001,    4, 2e-05)   + speedup_summary(-nan,     2, 3)};

      check(equality, "", postprocess(latest, reference), std::string_view{reference});
    }

    {
      const std::string latest   {duration_summary("Fast", 0.00145, 3, 0.0014)};
      const std::string reference{duration_summary("Fast", 0.00147, 4, 0.0011)};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      const std::string latest   {duration_summary("Slow", 0.0063, 3, 8.7e-06) + speedup_summary(1.04, 2, 3)};
      const std::string reference{duration_summary("Slow", 0.0064, 4, 9.1e-06) + speedup_summary(1.02, 2, 3)};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }

    {
      const std::string latest   {duration_summary("Slow", 0.0063, 4, 8.7e-06) + speedup_summary(1.04, 2, 3)};
      const std::string reference{duration_summary("Slow", 0.0064, 4, 9.1e-06) + speedup_summary(1.02, 2, 4)};

      check(equality, "", postprocess(latest, reference), std::string_view{latest});
    }
  }

  void performance_utilities_test::test_coarse_sleep()
  {
    using fractional_milliseconds = std::chrono::duration<double, std::milli>;

    constexpr fractional_milliseconds target{5.0};

    check("Rounded up to Windows' default tick", is_coarse_sleep(fractional_milliseconds{15.2}, target));
    check("Exactly twice the target", is_coarse_sleep(fractional_milliseconds{10.0}, target));
    check("Just under twice the target", !is_coarse_sleep(fractional_milliseconds{9.9}, target));
    check("A 1 ms timer resolution in effect", !is_coarse_sleep(fractional_milliseconds{5.4}, target));

    write_to_file(working_materials() /= "CoarseSleepMessage.txt",
                  coarse_sleep_message(fractional_milliseconds{15.2}, target),
                  std::ios_base::out);

    check(equivalence,
          "Message",
          working_materials() /= "CoarseSleepMessage.txt",
          predictive_materials() /= "CoarseSleepMessage.txt");
  }

  void performance_utilities_test::test_invalid_arguments()
  {
    constexpr double nan{std::numeric_limits<double>::quiet_NaN()};
    constexpr std::string_view description{"Relative performance with invalid arguments"};
    test_logger<test_mode::standard> logger{};

    auto relativePerformanceCheck{
      [&logger, description](const relative_performance_parameters& parameters) {
        return [&logger, description, parameters]() {
          return check_relative_performance(description, logger, []() {}, []() {}, parameters);
        };
      }
    };

    check_exception_thrown<std::invalid_argument>("Minimum speed-up of 1",
                                                  relativePerformanceCheck({.min_speedup{1.0},
                                                                            .max_speedup{2.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck({.min_speedup{1.5},
                                                                            .max_speedup{1.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck({.min_speedup{nan},
                                                                            .max_speedup{2.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck({.min_speedup{2.5},
                                                                            .max_speedup{2.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("One standard deviation",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{5},
                                                                            .num_sds{1.0},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("NaN standard deviations",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{5},
                                                                            .num_sds{nan},
                                                                            .max_attempts{3}}));
    check_exception_thrown<std::invalid_argument>("No attempts",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{5},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{0}}));
    check_exception_thrown<std::invalid_argument>("Four trials",
                                                  relativePerformanceCheck({.min_speedup{2.0},
                                                                            .max_speedup{3.0},
                                                                            .trials{4},
                                                                            .num_sds{4.0},
                                                                            .max_attempts{3}}));

    const auto& exitInfo{logger.last_check_exit_info()};
    if(check("An invalid argument is attributed to the check refusing it", exitInfo.has_value()))
    {
      check("Exit via an exception", exitInfo->via_exception);
      check(equality, "Message of the check refusing the argument", exitInfo->message, std::string{description});
    }
  }

  void performance_utilities_test::test_throwing_task()
  {
    constexpr std::string_view description{"Relative performance with a throwing task"};
    test_logger<test_mode::standard> logger{};

    auto checkWithThrowingFastTask{
      [&logger, description]() {
        return check_relative_performance(description, logger,
                                          []() { throw std::runtime_error{"Fast task failure"}; },
                                          []() {},
                                          {.min_speedup{2.0},
                                           .max_speedup{3.0},
                                           .trials{5},
                                           .num_sds{4.0},
                                           .max_attempts{3}});
      }
    };

    check_exception_thrown<std::runtime_error>("Fast task throws", checkWithThrowingFastTask);

    const auto& exitInfo{logger.last_check_exit_info()};
    if(check("A throwing task is attributed to its check", exitInfo.has_value()))
    {
      check("Exit via an exception", exitInfo->via_exception);
      check(equality, "Message of the check whose task threw", exitInfo->message, std::string{description});
    }
  }

  void performance_utilities_test::test_task_constraints()
  {
    STATIC_CHECK( profilable<   copyable_task>);
    STATIC_CHECK(!profilable<rvalue_only_task>);

    STATIC_CHECK( checkable_tasks_v<   copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<  move_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<   copyable_task,   move_only_task>);
    STATIC_CHECK(!checkable_tasks_v<rvalue_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_v<   copyable_task, rvalue_only_task>);

    STATIC_CHECK( checkable_tasks_by_extender_v<   copyable_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<  move_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<   copyable_task,   move_only_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<rvalue_only_task,    copyable_task>);
    STATIC_CHECK(!checkable_tasks_by_extender_v<   copyable_task, rvalue_only_task>);
  }
}
