////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "MoveOnlyTestDiagnostics.hpp"
#include "MoveOnlyTestDiagnosticsUtilities.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::filesystem::path move_only_false_negative_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void move_only_false_negative_diagnostics::run_tests()
  {
    test_move_only_semantics();
    test_as_unique_semantics();
  }

  void move_only_false_negative_diagnostics::test_move_only_semantics()
  {
    check_semantics("Broken equality",   move_only_broken_equality{1},    move_only_broken_equality{2},    move_only_broken_equality{1},    move_only_broken_equality{2});
    check_semantics("Broken inequality", move_only_broken_inequality{1},  move_only_broken_inequality{2},  move_only_broken_inequality{1},  move_only_broken_inequality{2});
    check_semantics("Broken move",  move_only_broken_move{1},  move_only_broken_move{2},  move_only_broken_move{1},  move_only_broken_move{2});
    check_semantics("Broken swap",  move_only_broken_swap{1},  move_only_broken_swap{2},  move_only_broken_swap{1},  move_only_broken_swap{2});
    check_semantics("Broken move assignment", move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2}, move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2});
    check_semantics("Broken check invariant", move_only_beast{1}, move_only_beast{1}, move_only_beast{1}, move_only_beast{1});
    check_semantics("Broken check invariant", move_only_beast{1}, move_only_beast{3}, move_only_beast{2}, move_only_beast{3});
    check_semantics("Broken check invariant", move_only_beast{2}, move_only_beast{1}, move_only_beast{2}, move_only_beast{3});

    test_binder<enable_serialization::no>();
    test_binder<enable_serialization::yes>();
  }

  void move_only_false_negative_diagnostics::test_as_unique_semantics()
  {
    check_semantics("Broken equality",   move_only_broken_equality{1},    move_only_broken_equality{2},   std::vector<int>{1}, std::vector<int>{2});
    check_semantics("Broken inequality", move_only_broken_inequality{1},  move_only_broken_inequality{2}, std::vector<int>{1}, std::vector<int>{2});
    check_semantics("Broken move",  move_only_broken_move{1},  move_only_broken_move{2}, std::vector<int>{1}, std::vector<int>{2});
    check_semantics("Broken swap",  move_only_broken_swap{1},  move_only_broken_swap{2}, std::vector<int>{1}, std::vector<int>{2});
    check_semantics("Broken move assignment", move_only_broken_move_assignment{1}, move_only_broken_move_assignment{2}, std::vector<int>{1}, std::vector<int>{2});
    check_semantics("Broken check invariant", move_only_beast{1}, move_only_beast{1}, std::vector<int>{1}, std::vector<int>{1});
    check_semantics("Broken check invariant", move_only_beast{1}, move_only_beast{3}, std::vector<int>{2}, std::vector<int>{3});
    check_semantics("Broken check invariant", move_only_beast{2}, move_only_beast{1}, std::vector<int>{2}, std::vector<int>{3});

    {
      using beast = move_only_specified_moved_from_beast<int>;
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, std::vector<int>{1}, std::vector<int>{2}, std::vector<int>{1}, std::vector<int>{});
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, beast{1}, beast{2}, std::vector<int>{1}, std::vector<int>{});
      check_semantics("Incorrect moved-from state post construction", beast{1}, beast{2}, beast{1}, beast{2}, beast{1}, beast{});
      check_semantics("Incorrect moved-from state post construction", [] () { return beast{1}; }, [] () { return beast{2}; }, std::vector<int>{1}, std::vector<int>{});
      check_semantics("Incorrect moved-from state post construction", [] () { return beast{1}; }, [] () { return beast{2}; }, beast{1}, beast{});

      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, std::vector<int>{1}, std::vector<int>{2}, std::vector<int>{}, std::vector<int>{2});
      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, beast{1}, beast{2}, std::vector<int>{}, std::vector<int>{2});
      check_semantics("Incorrect moved-from state post assignment", beast{1}, beast{2}, beast{1}, beast{2}, beast{}, beast{2});
      check_semantics("Incorrect moved-from state post assignment", [] () { return beast{1}; }, [] () { return beast{2}; }, std::vector<int>{}, std::vector<int>{2});
      check_semantics("Incorrect moved-from state post assignment", [] () { return beast{1}; }, [] () { return beast{2}; }, beast{}, beast{2});
    }
  }

  template<enable_serialization EnableSerialization>
  void move_only_false_negative_diagnostics::test_binder()
  {
    using binder_t = resource_binder<EnableSerialization>;
    check_semantics("Incorrect moved-from state post construction", binder_t{1}, binder_t{2}, binder_t{1}, binder_t{2}, binder_t{3}, binder_t{1});
    check_semantics("Incorrect moved-from state post construction", []() { return binder_t{1}; }, []() {return binder_t{2}; }, binder_t{3}, binder_t{1});

    check_semantics("Incorrect moved-from state post assignment", binder_t{1}, binder_t{2}, binder_t{1}, binder_t{2}, binder_t{3}, binder_t{1});
    check_semantics("Incorrect moved-from state post assignment", []() { return binder_t{1}; }, []() {return binder_t{2}; }, binder_t{0}, binder_t{3});
  }


  [[nodiscard]]
  std::filesystem::path move_only_false_positive_diagnostics::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void move_only_false_positive_diagnostics::run_tests()
  {
    test_move_only_semantics();
    test_as_unique_semantics();
  }

  void move_only_false_positive_diagnostics::test_move_only_semantics()
  {
    {
      using beast = move_only_beast<int>;
      check_semantics("", beast{1}, beast{2}, beast{1}, beast{2});
      check_semantics("Function object syntax", [](){ return beast{1}; }, [](){ return beast{2}; });
    }

    {
      using beast = move_only_specified_moved_from_beast<int>;
      check_semantics("Check moved-from state", beast{1}, beast{2}, std::vector<int>{1}, std::vector<int>{2}, std::vector<int>{}, std::vector<int>{});
    }
    
    {
      using binder_t = resource_binder<enable_serialization::yes>;
      check_semantics("Check moved-from state", binder_t{1}, binder_t{2}, binder_t{1}, binder_t{2}, binder_t{0}, binder_t{1});
      check_semantics("Check moved-from state", []() { return binder_t{1}; }, []() {return binder_t{2}; }, binder_t{0}, binder_t{1});
    }
  }

  void move_only_false_positive_diagnostics::test_as_unique_semantics()
  {
    using beast = move_only_beast<int>;
    check_semantics("", beast{1}, beast{2}, std::vector<int>{1}, std::vector<int>{2});
  }
}
