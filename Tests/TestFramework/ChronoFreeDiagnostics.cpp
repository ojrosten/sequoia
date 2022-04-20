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
  std::string_view chrono_false_positive_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void chrono_false_positive_free_diagnostics::run_tests()
  {
    using sec = std::chrono::seconds;

    check(equality, LINE(""), sec{}, sec{1});
  }
  
  [[nodiscard]]
  std::string_view chrono_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void chrono_false_negative_free_diagnostics::run_tests()
  {
    using sec = std::chrono::seconds;

    check(equality, LINE(""), sec{1}, sec{1});
  }
}
