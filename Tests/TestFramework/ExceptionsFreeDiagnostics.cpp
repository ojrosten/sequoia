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
    check_exception_thrown<std::runtime_error>(report_line("Exception expected but nothing thrown"), []() {});
    check_exception_thrown<std::runtime_error>(report_line("Exception thrown but of wrong type"), []() { throw std::logic_error("Error"); });
    check_exception_thrown<std::runtime_error>(report_line("Exception thrown but of unknown type"), []() { throw 1; });
  }
  
  [[nodiscard]]
  std::string_view exceptions_false_negative_free_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void exceptions_false_negative_free_diagnostics::run_tests()
  {
    check_exception_thrown<std::runtime_error>(report_line(""), []() { throw std::runtime_error("Error"); });

    check_exception_thrown<int>(report_line(""), []() { throw 1; });
  }
}
