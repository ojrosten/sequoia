////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/CoarseSleeps.hpp"
#include "sequoia/Parsing/CommandLineMessages.hpp"

#include <format>

namespace sequoia::testing
{
  [[nodiscard]]
  bool is_coarse_sleep(floating_point_milliseconds slept, floating_point_milliseconds target)
  {
    return slept >= 2 * target;
  }

  [[nodiscard]]
  std::string coarse_sleep_message(floating_point_milliseconds slept, floating_point_milliseconds target)
  {
    using parsing::commandline::warning;
    return warning({std::format("Sleeps of {:.1f} ms repeatedly lasted {:.1f} ms or more, "
                                "so timings built on sleeps are unreliable",
                                target.count(),
                                slept.count()),
                    "On Windows, the likely cause is that the finest timer resolution is not in effect"});
  }
}
