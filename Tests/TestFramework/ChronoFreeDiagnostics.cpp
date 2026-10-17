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
#include <ostream>

namespace
{
  /** \brief A clock whose time points only a client's `operator<<` can
             print: the standard library can neither format nor stream them.
   */
  struct streamed_clock
  {
    using rep        = long long;
    using period     = std::milli;
    using duration   = std::chrono::milliseconds;
    using time_point = std::chrono::time_point<streamed_clock, duration>;

    [[maybe_unused]] static constexpr bool is_steady{true};

    static time_point now();
  };

  std::ostream& operator<<(std::ostream& stream, const streamed_clock::time_point& timePoint)
  {
    return stream << timePoint.time_since_epoch().count() << " ticks of the streamed clock";
  }
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

    check(equality, "Duration", sec{}, sec{1});
    check(equality, "Duration with advice", sec{}, sec{1}, tutor{[](sec, sec) { return "Temporal advice"; }});

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    check(equality, "Steady time point", TimePoint{sec{}}, TimePoint{sec{1}});
    check(equality,
          "Steady time point with advice",
          TimePoint{sec{}},
          TimePoint{sec{1}},
          tutor{[](TimePoint, TimePoint) { return "Advice for time_point"; }});

    check(equality, "System time point", std::chrono::sys_seconds{}, std::chrono::sys_seconds{sec{1}});

    using streamed_time_point = streamed_clock::time_point;
    check(equality, "Time point which a client streams", streamed_time_point{}, streamed_time_point{sec{1}});
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
    check(equality, "Steady time point", TimePoint{sec{1}}, TimePoint{sec{1}});
    check(equality, "System time point", std::chrono::sys_seconds{sec{1}}, std::chrono::sys_seconds{sec{1}});

    using streamed_time_point = streamed_clock::time_point;
    check(equality, "Time point which a client streams", streamed_time_point{sec{1}}, streamed_time_point{sec{1}});

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
