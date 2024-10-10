////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "ComplexFreeDiagnostics.hpp"
#include "sequoia/TestFramework/ConcreteTypeCheckers.hpp"

#include <complex>

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path complex_false_positive_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void complex_false_positive_free_diagnostics::run_tests()
  {
    using complex = std::complex<double>;

    check(equality, "", complex{}, complex{1.0});
    check(equality, "", complex{}, complex{1.0, 2.0});
    check(equality, "", complex{}, complex{1.0, 2.0}, tutor{[](complex, complex) { return "complex advice, no pun intended"; }});
  }
  
  [[nodiscard]]
  std::filesystem::path complex_false_negative_free_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void complex_false_negative_free_diagnostics::run_tests()
  {
    using complex = std::complex<double>;

    check(equality, "", complex{1.0}, complex{1.0});
  }
}
