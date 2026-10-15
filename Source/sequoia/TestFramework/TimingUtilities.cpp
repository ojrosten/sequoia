////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/TimingUtilities.hpp"
#include "sequoia/Parsing/CommandLineMessages.hpp"

#include <format>

namespace sequoia::testing
{
  timer::timer()
    : m_Start{std::chrono::steady_clock::now()}
  {}

  [[nodiscard]]
  std::chrono::nanoseconds timer::time_elapsed() const
  {
    return std::chrono::steady_clock::now() - m_Start;
  }

  [[nodiscard]]
  bool is_coarse_sleep(std::chrono::duration<double, std::milli> slept,
                       std::chrono::duration<double, std::milli> target)
  {
    return slept >= 2 * target;
  }

  [[nodiscard]]
  std::string coarse_sleep_message(std::chrono::duration<double, std::milli> slept,
                                   std::chrono::duration<double, std::milli> target)
  {
    using parsing::commandline::warning;
    return warning({std::format("Sleeps of {:.1f} ms repeatedly lasted {:.1f} ms or more, "
                                "so timings built on sleeps are unreliable",
                                target.count(),
                                slept.count()),
                    "On Windows, the likely cause is that the finest timer resolution is not in effect"});
  }
}
