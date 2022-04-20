////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ExceptionsFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view exceptions_false_positive_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void exceptions_false_positive_free_diagnostics::run_tests()
  {
    // e.g. check(equality, LINE("Useful description"), some_function(), 42);
  }
  
  [[nodiscard]]
  std::string_view exceptions_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void exceptions_false_negative_free_diagnostics::run_tests()
  {
    // e.g. check(equality, LINE("Useful description"), some_function(), 42);
  }
}
