////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ChronoFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

#include <chrono>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path chrono_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void chrono_false_positive_free_diagnostics::run_tests()
  {
    using sec = std::chrono::seconds;
    using ns = std::chrono::nanoseconds;

    check(equality, report("Duration"), sec{}, sec{1});
    check(equality, report("Duration with advice"), sec{}, sec{1}, tutor{[](sec, sec) { return "Temporal advice"; }});

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    check(equality, report("Time point"), TimePoint{sec{}}, TimePoint{sec{1}});
      check(equality, report("Time point"), TimePoint{sec{}}, TimePoint{sec{1}}, tutor{[](ns, ns) { return "Advice for time_point needs to be in nanoseconds"; }});
  }
  
  [[nodiscard]]
  std::filesystem::path chrono_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void chrono_false_negative_free_diagnostics::run_tests()
  {
    using sec = std::chrono::seconds;

    check(equality, report(""), sec{1}, sec{1});

    using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
    check(equality, report("Time point"), TimePoint{sec{1}}, TimePoint{sec{1}});
  }
}
