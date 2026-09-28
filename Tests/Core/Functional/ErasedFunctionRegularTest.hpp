////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "ErasedFunctionTestingUtilities.hpp"

namespace sequoia::testing
{
  class erased_function_regular_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    void test_constraints();

    void test_admission();

    void test_qualified_construction();

    void test_noexcept();

    void test_interface();

    void test_constant_evaluation();

    /** Each operation has a distinct implementation for each way a target is held: in the buffer
        and trivially managed, in the buffer with a manager of its own, on the heap, and in a
        constant evaluation behind the pointer. The scenarios here pair the first three, and empty,
        so that copies, moves and swaps between them are checked too; `test_constant_evaluation`
        covers the fourth.
     */
    void test_semantics();

    void test_empty_invocation();

    void test_self_move_assignment();

    void test_throwing_copy_assignment();

    void test_small_target();

    void test_large_target();

    void test_small_target_with_throwing_move();

    void test_arguments();

    void test_conversion();

    void test_qualified_invocation();

    void test_construction_from_lvalue();

    void test_in_place_construction();

    void test_null();

    void test_swap();
  };
}
