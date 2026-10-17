////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "ChronoFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

#include <chrono>
#include <filesystem>
#include <format>
#include <limits>

namespace
{
  // Year 0 is beyond the range of a 64-bit count of nanoseconds
  constexpr std::chrono::sys_seconds year_zero{std::chrono::sys_days{std::chrono::year{0} / 1 / 1}};
}

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path chrono_false_negative_free_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void chrono_false_negative_free_diagnostics::run_tests()
  {
    using sec = std::chrono::seconds;
    using ns = std::chrono::nanoseconds;

    check(equality, "Duration", sec{}, sec{1});
    check(equality, "Duration with advice", sec{}, sec{1}, tutor{[](sec, sec) { return "Temporal advice"; }});

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    check(equality, "Time point", TimePoint{sec{}}, TimePoint{sec{1}});
    check(equality,
          "Time point",
          TimePoint{sec{}},
          TimePoint{sec{1}},
          tutor{[](ns, ns) { return "Advice for time_point is given the times since the epoch"; }});

    // The separation is 1953125 x 2^64 ns, so a 64-bit count of nanoseconds,
    // wrapping on overflow, would not distinguish time points that far apart
    constexpr sec separation{sec::rep{1} << 55};
    check(equality, "Time points beyond the range of a count of nanoseconds", year_zero, year_zero + separation);
  }
  
  [[nodiscard]]
  std::filesystem::path chrono_false_positive_free_diagnostics::source_file()
  {
    return std::source_location::current().file_name();
  }

  void chrono_false_positive_free_diagnostics::run_tests()
  {
    using sec = std::chrono::seconds;

    check(equality, "", sec{1}, sec{1});

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    check(equality, "Time point", TimePoint{sec{1}}, TimePoint{sec{1}});
    check(equality, "Time point beyond the range of a count of nanoseconds", year_zero, year_zero);

    // libc++ counts file times in an __int128, and cannot stream an __int128.
    // On libc++, this check fails to compile without the serializer's
    // specialization for std::chrono::duration. The greatest count exceeds a
    // long long there, so a count narrowed before printing fails the check.
    // Elsewhere the generic serializer prints the duration.
    using file_duration = std::filesystem::file_time_type::duration;
    constexpr auto greatest{std::numeric_limits<file_duration::rep>::max()};
    check(equality,
          "A file time's duration prints as its count followed by its units suffix",
          serializer<file_duration>::make(file_duration{greatest}),
          std::format("{}{:%q}", greatest, std::chrono::duration<long long, file_duration::period>{}));
  }
}
