////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "RunStampTestingDiagnostics.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path run_stamp_false_negative_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void run_stamp_false_negative_test::run_tests()
  {
    using namespace std::chrono_literals;

    check(equality, "Start times differ", run_stamp{}, run_stamp{.start_time{run_stamp::start_time_type{1ms}}});
  }
}
