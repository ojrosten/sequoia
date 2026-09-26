////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SummaryFreeTest.hpp"
#include "sequoia/TestFramework/Summary.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path summary_free_test::source_file()
  {
    return std::source_location::current().file_name();
  }

  void summary_free_test::run_tests()
  {
    test_stringify_duration();
  }

  void summary_free_test::test_stringify_duration()
  {
    using namespace std::chrono;
    using namespace std::string_literals;

    auto stringified{
      [](nanoseconds d) {
        const auto [time, unit]{stringify_duration(d)};
        return time + unit;
      }
    };

    check(equality, "No time",                                  stringified(0ns),                 "0ns"s);
    check(equality, "The most nanoseconds",                     stringified(999ns),               "999ns"s);
    check(equality, "The fewest microseconds",                  stringified(1'000ns),             "1us"s);
    check(equality, "Microseconds, to three figures",           stringified(1'234ns),             "1.23us"s);
    check(equality, "Microseconds, a tie rounded up",           stringified(1'125ns),             "1.13us"s);
    check(equality, "The most microseconds, rounded down",      stringified(999'499ns),           "999us"s);
    check(equality, "Microseconds rounded up to a millisecond", stringified(999'500ns),           "1ms"s);
    check(equality, "The fewest milliseconds",                  stringified(1'000'000ns),         "1ms"s);
    check(equality, "Milliseconds, to three figures",           stringified(12'345'678ns),        "12.3ms"s);
    check(equality, "The most milliseconds, rounded down",      stringified(999'499'999ns),       "999ms"s);
    check(equality, "Milliseconds rounded up to a second",      stringified(999'500'000ns),       "1s"s);
    check(equality, "The fewest seconds",                       stringified(1'000'000'000ns),     "1s"s);
    check(equality, "Seconds, to three figures",                stringified(1'234'567'890ns),     "1.23s"s);
    check(equality, "A thousand seconds and more",              stringified(1'234'567'890'000ns), "1230s"s);
    check(equality, "A day",                                    stringified(24h),                 "86400s"s);
    check(equality, "A hundred thousand seconds",               stringified(100'000s),            "100000s"s);
    check(equality, "The longest duration",                     stringified(nanoseconds::max()),  "9220000000s"s);
  }
}
