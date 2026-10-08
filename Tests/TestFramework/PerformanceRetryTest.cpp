////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "PerformanceRetryTest.hpp"
#include "PerformanceTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    constexpr std::chrono::milliseconds spin_unit{2};

    constexpr relative_performance_parameters retry_parameters{
      .min_speedup{1.8}, .max_speedup{2.2}, .trials{10}, .num_sds{4}, .max_attempts{3}
    };
  }

  [[nodiscard]]
  std::filesystem::path performance_retry_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_retry_test::run_tests()
  {
    test_stop_at_first_passing_attempt();
    test_failure_when_attempts_run_out();
    test_false_negative_stop_at_first_failing_attempt();
  }

  void performance_retry_test::test_stop_at_first_passing_attempt()
  {
    test_logger<test_mode::standard> logger{};
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.trials, 2 * spin_unit};

    check("The check passes", testing::check_relative_performance("", logger, fast, slow, retry_parameters));
    check(equality, "No performance failure logged", logger.results().performance_failures, 0uz);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * (1 + 2));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * (1 + 2));
  }

  void performance_retry_test::test_failure_when_attempts_run_out()
  {
    test_logger<test_mode::standard> logger{};
    const counted_spinner fast{spin_unit},
                          slow{spin_unit};

    check("The check fails", !testing::check_relative_performance("", logger, fast, slow, retry_parameters));
    check(equality, "One performance failure logged", logger.results().performance_failures, 1uz);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * (1 + 2 + 3));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * (1 + 2 + 3));
  }

  void performance_retry_test::test_false_negative_stop_at_first_failing_attempt()
  {
    test_logger<test_mode::false_negative> logger{};
    const counted_spinner fast{spin_unit},
                          slow{2 * spin_unit, retry_parameters.trials, spin_unit};

    check("The check fails", !testing::check_relative_performance("", logger, fast, slow, retry_parameters));
    check(equality, "One performance failure logged", logger.results().performance_failures, 1uz);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * (1 + 2));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * (1 + 2));
  }
}
