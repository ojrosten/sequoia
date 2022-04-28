////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class elementary_false_positive_free_diagnostics final : public free_false_positive_test
  {
  public:
    using free_false_positive_test::free_false_positive_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void built_in_type_tests();
    void test_equality_checks();
    void test_equivalence_checks();
    void test_weak_equivalence_checks();
    void test_with_best_available_checks();
  };

  class elementary_false_negative_free_diagnostics final : public free_false_negative_test
  {
  public:
    using free_false_negative_test::free_false_negative_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void built_in_type_tests();
    void test_equality_checks();
    void test_equivalence_checks();
    void test_weak_equivalence_checks();
    void test_with_best_available_checks();
  };
}
