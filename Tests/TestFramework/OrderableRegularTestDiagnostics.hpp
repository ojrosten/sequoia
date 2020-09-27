////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

namespace sequoia:: testing
{
  class orderable_false_positive_regular_diagnostics final : public false_positive_regular_test
  {
  public:
    using false_positive_regular_test::false_positive_regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_regular_semantics();
  };

  class orderable_false_negative_regular_diagnostics final : public false_negative_regular_test
  {
  public:
    using false_negative_regular_test::false_negative_regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_regular_semantics();
  };
}
