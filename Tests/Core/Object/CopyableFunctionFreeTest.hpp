////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class copyable_function_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:
    /** The storage paths are the thing to exercise separately: a target which fits the small
        buffer is constructed in place, one which does not is held by pointer, and in a constant
        evaluation every one is held by pointer; each of the type's operations has a distinct
        implementation for each. The lifetime is run over the first two here, and in
        `test_constant_evaluation` over the third.
     */
    template<class Fn>
    void test_lifetime(std::string_view description, Fn fn, int expected);

    void test_small_target();

    void test_large_target();

    void test_arguments();

    void test_conversion();

    void test_constraints();

    void test_swap();

    void test_constant_evaluation();
  };
}
