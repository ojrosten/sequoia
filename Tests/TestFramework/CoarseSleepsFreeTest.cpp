////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "CoarseSleepsFreeTest.hpp"
#include "sequoia/TestFramework/CoarseSleeps.hpp"
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
    constexpr floating_point_milliseconds target{5.0};

    check("Rounded up to Windows' default tick", is_coarse_sleep(floating_point_milliseconds{15.2}, target));
    check("Exactly twice the target", is_coarse_sleep(floating_point_milliseconds{10.0}, target));
    check("Just under twice the target", !is_coarse_sleep(floating_point_milliseconds{9.9}, target));
    check("A 1 ms timer resolution in effect", !is_coarse_sleep(floating_point_milliseconds{5.4}, target));

    write_to_file(working_materials() /= "CoarseSleepMessage.txt",
                  coarse_sleep_message(floating_point_milliseconds{15.2}, target),
                  std::ios_base::out);

    check(equivalence,
          "Message",
          working_materials() /= "CoarseSleepMessage.txt",
          predictive_materials() /= "CoarseSleepMessage.txt");
  }
}
