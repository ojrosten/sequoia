////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  class regular_state_transition_false_negative_diagnostics final : public regular_false_negative_test
  {
  public:
    using regular_false_negative_test::regular_false_negative_test;

  private:
    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;

    void run_tests() final;

    void test_orderable();
    void test_equality_comparable();
  };

  class regular_state_transition_false_positive_diagnostics final : public regular_false_positive_test
  {
  public:
    using regular_false_positive_test::regular_false_positive_test;

  private:
    [[nodiscard]]
    std::filesystem::path source_file() const noexcept final;

    void run_tests() final;

    void test_orderable();
    void test_equality_comparable();
    void test_broken_constructor();
  };
}
