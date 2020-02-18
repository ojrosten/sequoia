////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "FuzzyTestDiagnostics.hpp"

namespace sequoia::unit_testing
{
  [[nodiscard]]
  std::string_view fuzzy_false_positive_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void fuzzy_false_positive_diagnostics::run_tests()
  {
    basic_tests();
  }

  void fuzzy_false_positive_diagnostics::basic_tests()
  {  
    check_approx_equality(LINE(""), 3.0, 5.0, within_tolerance{1.0});
    check_approx_equality(LINE(""), 7.0, 5.0, within_tolerance{1.0});
  }


  [[nodiscard]]
  std::string_view fuzzy_false_negative_diagnostics::source_file_name() const noexcept
  {
    return __FILE__;
  }

  void fuzzy_false_negative_diagnostics::run_tests()
  {
    basic_tests();
  }

  void fuzzy_false_negative_diagnostics::basic_tests()
  {
    check_approx_equality(LINE(""), 4.5, 5.0, within_tolerance{1.0});
    check_approx_equality(LINE(""), 5.5, 5.0, within_tolerance{1.0});

    check_approx_equality(LINE(""), 4.5, 5.0, within_tolerance{0.5});
    check_approx_equality(LINE(""), 5.5, 5.0, within_tolerance{0.5});
  }
}
