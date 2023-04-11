////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularTestDiagnostics.hpp"
#include "ContainerDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view regular_false_positive_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void regular_false_positive_diagnostics::run_tests()
  {
    
  }

  void regular_false_positive_diagnostics::test_regular_semantics()
  {
    check_semantics(report_line("Broken check invariant"), perfectly_normal_beast{1}, perfectly_normal_beast{1});
    check_semantics(report_line("Broken equality"), broken_equality{1}, broken_equality{2});
    check_semantics(report_line("Broken inequality"), broken_inequality{1}, broken_inequality{2});
    check_semantics(report_line("Broken copy"), broken_copy{1}, broken_copy{2});
    check_semantics(report_line("Broken move"), broken_move{1}, broken_move{2});
    check_semantics(report_line("Broken copy assignment"), broken_copy_assignment{1}, broken_copy_assignment{2});
    check_semantics(report_line("Broken move assignment"), broken_move_assignment{1}, broken_move_assignment{2});
    check_semantics(report_line("Broken self copy assignment"), broken_self_copy_assignment{1}, broken_self_copy_assignment{2});
    check_semantics(report_line("Broken swap"), broken_swap{1}, broken_swap{2});
    check_semantics(report_line("Broken self swap"), broken_self_swap{1}, broken_self_swap{2});
    check_semantics(report_line("Broken copy value semantics"), broken_copy_value_semantics{1}, broken_copy_value_semantics{2}, [](auto& b) { *b.x.front() = 3; });
    check_semantics(report_line("Broken copy assignment value semantics"), broken_copy_assignment_value_semantics{1}, broken_copy_assignment_value_semantics{2}, [](auto& b) { *b.x.front() = 3; });
    check_semantics(report_line("Broken serialization"), broken_serialization{1}, broken_serialization{2});
    check_semantics(report_line("Broken deserialization"), broken_deserialization{1}, broken_deserialization{2});
  }

  [[nodiscard]]
  std::string_view regular_false_negative_diagnostics::source_file() const noexcept
  {
    return __FILE__;
  }

  void regular_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void regular_false_negative_diagnostics::test_regular_semantics()
  {
    check_semantics(report_line(""), perfectly_normal_beast{1}, perfectly_normal_beast{2});
    check_semantics(report_line(""), perfectly_stringy_beast{}, perfectly_stringy_beast{"Hello, world"});
  }
}
