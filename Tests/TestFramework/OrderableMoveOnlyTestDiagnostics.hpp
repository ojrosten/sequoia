////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"

namespace sequoia:: testing
{
  class orderable_move_only_false_negative_diagnostics final : public move_only_false_negative_test
  {
  public:
    using move_only_false_negative_test::move_only_false_negative_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_move_only_semantics();
    void test_as_unique_semantics();
  };

  class orderable_move_only_false_positive_diagnostics final : public move_only_false_positive_test
  {
  public:
    using move_only_false_positive_test::move_only_false_positive_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    void test_move_only_semantics();
    void test_as_unique_semantics();
  };
}
