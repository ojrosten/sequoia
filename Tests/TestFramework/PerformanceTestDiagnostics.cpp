////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceTestDiagnostics.hpp"

#include "sequoia/Streaming/Streaming.hpp"

#include <limits>
#include <thread>

namespace sequoia::testing
{
  namespace
  {
    void wait(std::chrono::milliseconds t)
    {
      std::this_thread::sleep_for(t);
    }
  }

  [[nodiscard]]
  std::filesystem::path performance_false_negative_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_negative_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_negative_diagnostics::test_relative_performance()
  {
    constexpr std::chrono::milliseconds deltaT{5};

    check_relative_performance("Performance Test for which fast task is too slow, [1, (2.0, 2.0)",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(deltaT); }, 2.0, 2.0);

    check_relative_performance("Performance Test for which fast task is too slow [1, (2.0, 3.0)",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(deltaT); }, 2.0, 3.0);

    check_relative_performance("Performance Test for which fast task is too fast [4, (2.0, 2.5)]",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(4 * deltaT); }, 2.0, 2.5);
  }

  [[nodiscard]]
  std::filesystem::path performance_false_positive_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_false_positive_diagnostics::run_tests()
  {
    test_relative_performance();
  }

  void performance_false_positive_diagnostics::test_relative_performance()
  {
    constexpr std::chrono::milliseconds deltaT{5};

    check_relative_performance("Performance Test which should pass",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(2 * deltaT); }, 1.8, 2.1, 5);

    check_relative_performance("Performance Test which should pass",
                               [deltaT]() { wait(deltaT); },
                               [deltaT]() { wait(4 * deltaT); }, 3.4, 4.1, 5);
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
      [&logger, description](double minSpeedUp, double maxSpeedUp, std::size_t trials, double numSds,
                             std::size_t maxAttempts) {
        return [&logger, description, minSpeedUp, maxSpeedUp, trials, numSds, maxAttempts]() {
          return check_relative_performance(description, logger, []() {}, []() {},
                                            minSpeedUp, maxSpeedUp, trials, numSds, maxAttempts);
        };
      }
    };

    check_exception_thrown<std::invalid_argument>("Minimum speed-up of 1",
                                                  relativePerformanceCheck(1.0, 2.0, 5, 4.0, 3));
    check_exception_thrown<std::invalid_argument>("Maximum speed-up of 1",
                                                  relativePerformanceCheck(1.5, 1.0, 5, 4.0, 3));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up of NaN",
                                                  relativePerformanceCheck(nan, 2.0, 5, 4.0, 3));
    check_exception_thrown<std::invalid_argument>("Minimum speed-up exceeding the maximum",
                                                  relativePerformanceCheck(2.5, 2.0, 5, 4.0, 3));
    check_exception_thrown<std::invalid_argument>("One standard deviation",
                                                  relativePerformanceCheck(2.0, 3.0, 5, 1.0, 3));
    check_exception_thrown<std::invalid_argument>("NaN standard deviations",
                                                  relativePerformanceCheck(2.0, 3.0, 5, nan, 3));
    check_exception_thrown<std::invalid_argument>("No attempts",
                                                  relativePerformanceCheck(2.0, 3.0, 5, 4.0, 0));
    check_exception_thrown<std::invalid_argument>("Four trials",
                                                  relativePerformanceCheck(2.0, 3.0, 4, 4.0, 3));

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
                                          2.0, 3.0, 5, 4.0, 3);
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
}
