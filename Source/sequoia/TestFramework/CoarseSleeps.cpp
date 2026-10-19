////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "sequoia/TestFramework/CoarseSleeps.hpp"
#include "sequoia/Parsing/CommandLineMessages.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <format>
#include <string_view>

namespace sequoia::testing
{
  namespace
  {
    [[nodiscard]]
    std::string coarse_sleep_message_with_cause(double_precision_milliseconds slept,
                                                double_precision_milliseconds target,
                                                std::string_view likelyCause)
    {
      using parsing::commandline::warning;
      return warning({std::format("Sleeps of {:.1f} ms repeatedly lasted {:.1f} ms or more, "
                                  "so timings built on sleeps are unreliable",
                                  target.count(),
                                  slept.count()),
                      std::format("The likely cause is that {}", likelyCause)});
    }
  }

  [[nodiscard]]
  bool is_coarse_sleep(double_precision_milliseconds slept, double_precision_milliseconds target)
  {
    return slept >= 2 * target;
  }

  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept, double_precision_milliseconds target)
  {
    return coarse_sleep_message(slept, target, platform_constant{});
  }

  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   windows_type)
  {
    return coarse_sleep_message_with_cause(slept, target, "the finest timer resolution is not in effect");
  }

  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   macos_type)
  {
    return coarse_sleep_message_with_cause(slept, target, "the machine is heavily loaded");
  }

  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   linux_type)
  {
    return coarse_sleep_message_with_cause(slept, target, "the machine is heavily loaded");
  }

  [[nodiscard]]
  std::string coarse_sleep_message(double_precision_milliseconds slept,
                                   double_precision_milliseconds target,
                                   other_os_type)
  {
    return coarse_sleep_message_with_cause(slept, target, "the machine is heavily loaded");
  }
}
