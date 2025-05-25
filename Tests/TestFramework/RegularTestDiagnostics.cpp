////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "RegularTestDiagnostics.hpp"
#include "RegularTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path regular_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void regular_false_negative_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void regular_false_negative_diagnostics::test_regular_semantics()
  {
    check_semantics("Broken check invariant", perfectly_normal_beast{1}, perfectly_normal_beast{1});
    check_semantics("Broken equality", broken_equality{1}, broken_equality{2});
    check_semantics("Broken inequality", broken_inequality{1}, broken_inequality{2});
    check_semantics("Broken copy", broken_copy{1}, broken_copy{2});
    check_semantics("Broken move", broken_move{1}, broken_move{2});
    check_semantics("Broken copy assignment", broken_copy_assignment{1}, broken_copy_assignment{2});
    check_semantics("Broken move assignment", broken_move_assignment{1}, broken_move_assignment{2});
    check_semantics("Broken self copy assignment", broken_self_copy_assignment{1}, broken_self_copy_assignment{2});
    check_semantics("Broken swap", broken_swap{1}, broken_swap{2});
    check_semantics("Broken self swap", broken_self_swap{1}, broken_self_swap{2});
    check_semantics("Broken copy value semantics", broken_copy_value_semantics{1}, broken_copy_value_semantics{2}, [](auto& b) { *b.x.front() = 3; });
    check_semantics("Broken copy assignment value semantics", broken_copy_assignment_value_semantics{1}, broken_copy_assignment_value_semantics{2}, [](auto& b) { *b.x.front() = 3; });
    check_semantics("Broken serialization", broken_serialization{1}, broken_serialization{2});
    check_semantics("Broken deserialization", broken_deserialization{1}, broken_deserialization{2});

    using equiv_t = std::initializer_list<int>;
    check_semantics("Incorrect x value", perfectly_normal_beast{1}, perfectly_normal_beast{2}, equiv_t{2}, equiv_t{2});
    check_semantics("Incorrect y value", perfectly_normal_beast{1}, perfectly_normal_beast{2}, equiv_t{1}, equiv_t{3});
    check_semantics("Incorrect x value; mutator", perfectly_normal_beast{1}, perfectly_normal_beast{2}, equiv_t{2}, equiv_t{2}, [](auto& b) { b.x.front() = 3; });
    check_semantics("Incorrect y value; mutator", perfectly_normal_beast{1}, perfectly_normal_beast{2}, equiv_t{1}, equiv_t{3}, [](auto& b) { b.x.front() = 3; });

    {
      using beast   = specified_moved_from_beast<int>;
      using equiv_t = std::vector<int>;
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, equiv_t{1}, equiv_t{});
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, equiv_t{1}, equiv_t{2},   beast{1},   beast{});

      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, equiv_t{1}, equiv_t{2}, equiv_t{}, equiv_t{2});
      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, equiv_t{1}, equiv_t{2},   beast{},   beast{2});
    }
  }

  [[nodiscard]]
  std::filesystem::path regular_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void regular_false_positive_diagnostics::run_tests()
  {
    test_regular_semantics();
  }

  void regular_false_positive_diagnostics::test_regular_semantics()
  {
    using equiv_t = std::initializer_list<int>;
    check_semantics("", perfectly_normal_beast{1}, perfectly_normal_beast{2});
    check_semantics("", perfectly_normal_beast{1}, perfectly_normal_beast{2}, equiv_t{1}, equiv_t{2});
    check_semantics("", perfectly_normal_beast{1}, perfectly_normal_beast{2}, equiv_t{1}, equiv_t{2}, [](auto& b) { b.x.front() = 3; } );
    check_semantics("", perfectly_stringy_beast{}, perfectly_stringy_beast{"Hello, world"});
  }
}
