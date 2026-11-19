////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "SummaryFreeTest.hpp"
#include "sequoia/TestFramework/Summary.hpp"
#include "sequoia/Streaming/Streaming.hpp"

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
    test_durations_report();
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
    check(equality, "Microseconds, a tie",                      stringified(1'125ns),             "1.12us"s);
    check(equality, "The most microseconds, rounded down",      stringified(999'499ns),           "999us"s);
    check(equality, "Microseconds rounded up to a millisecond", stringified(999'500ns),           "1ms"s);
    check(equality, "Milliseconds, to three figures",           stringified(12'345'678ns),        "12.3ms"s);
    check(equality, "The most milliseconds, rounded down",      stringified(999'499'999ns),       "999ms"s);
    check(equality, "Milliseconds rounded up to a second",      stringified(999'500'000ns),       "1s"s);
    check(equality, "Seconds, to three figures",                stringified(1'234'567'890ns),     "1.23s"s);
    check(equality, "A thousand seconds and more",              stringified(1'234'567'890'000ns), "1.23e+03s"s);
    check(equality, "The longest duration",                     stringified(nanoseconds::max()),  "9.22e+09s"s);
  }

  /** The run's durations differ from one another and from each sum and
      difference of two of them, so a report that prints the wrong one shows
      a different number.
   */
  void summary_free_test::test_durations_report()
  {
    using namespace std::chrono;

    constexpr durations_of_run durationsOfRun{
      .total_run_time{700ms},
      .critical_path{
        .spent_in_tests{.execution_duration{110ms}, .runner_overhead{230ms}},
        .scheduling_overhead{40ms}
      }
    };

    const auto reportFile{working_materials() /= "DurationsReport.txt"};
    write_to_file(reportFile, durations_report(durationsOfRun), std::ios_base::out);
    check(equivalence, "The report of a run's durations", reportFile, predictive_materials() /= "DurationsReport.txt");
  }
}
