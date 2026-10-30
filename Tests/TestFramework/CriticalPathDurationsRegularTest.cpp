////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CriticalPathDurationsRegularTest.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path critical_path_durations_regular_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void critical_path_durations_regular_test::run_tests()
  {
    using namespace std::chrono_literals;

    check_semantics("Default durations and non-zero ones",
                    critical_path_durations{},
                    critical_path_durations{
                      .spent_in_tests     {.execution_duration{2ms}, .runner_overhead{1ms}},
                      .scheduling_overhead{3ms}
                    });
  }
}
