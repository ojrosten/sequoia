////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoarseSleepsFreeTest.hpp"
#include "sequoia/TestFramework/CoarseSleeps.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"
#include "sequoia/Streaming/Streaming.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path coarse_sleeps_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void coarse_sleeps_free_test::run_tests()
  {
    test_coarse_sleep();
  }

  void coarse_sleeps_free_test::test_coarse_sleep()
  {
    constexpr double_precision_milliseconds target{5.0};

    check("Rounded up to Windows' default tick", is_coarse_sleep(double_precision_milliseconds{15.2}, target));
    check("Exactly twice the target", is_coarse_sleep(double_precision_milliseconds{10.0}, target));
    check("Just under twice the target", !is_coarse_sleep(double_precision_milliseconds{9.9}, target));
    check("A 1 ms timer resolution in effect", !is_coarse_sleep(double_precision_milliseconds{5.4}, target));

    constexpr double_precision_milliseconds slept{15.2};

    auto checkMessage{
      [this](const reporter& description, const std::string& message, std::string_view fileName) {
        write_to_file(working_materials() /= fileName, message, std::ios_base::out);
        check(equivalence, description, working_materials() /= fileName, predictive_materials() /= fileName);
      }
    };

    checkMessage("Message on Windows",
                 coarse_sleep_message(slept, target, windows_type{}),
                 "WindowsCoarseSleepMessage.txt");

    checkMessage("Message on macOS",
                 coarse_sleep_message(slept, target, macos_type{}),
                 "MacOSCoarseSleepMessage.txt");

    checkMessage("Message on Linux",
                 coarse_sleep_message(slept, target, linux_type{}),
                 "LinuxCoarseSleepMessage.txt");

    checkMessage("Message on any other platform",
                 coarse_sleep_message(slept, target, other_os_type{}),
                 "OtherOSCoarseSleepMessage.txt");

    check(equality,
          "By default, the message is that of the platform the program is built for",
          coarse_sleep_message(slept, target),
          coarse_sleep_message(slept, target, platform_constant{}));
  }
}
