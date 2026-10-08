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
      .min_speedup{1.8}, .max_speedup{2.2}, .trials{10}
    };
  }

  [[nodiscard]]
  std::filesystem::path performance_retry_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void performance_retry_test::run_tests()
  {
    test_pass_at_second_attempt();
    test_pass_at_third_attempt();
  }

  void performance_retry_test::test_pass_at_second_attempt()
  {
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.trials, 2 * spin_unit};

    check_relative_performance("A speed-up of 1, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * (1 + 2));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * (1 + 2));
  }

  void performance_retry_test::test_pass_at_third_attempt()
  {
    const counted_spinner fast{spin_unit},
                          slow{spin_unit, retry_parameters.trials * (1 + 2), 2 * spin_unit};

    check_relative_performance("A speed-up of 1 twice, then of 2", fast, slow, retry_parameters);
    check(equality, "Calls of the fast task", fast.calls(), retry_parameters.trials * (1 + 2 + 3));
    check(equality, "Calls of the slow task", slow.calls(), retry_parameters.trials * (1 + 2 + 3));
  }
}
