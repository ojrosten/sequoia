//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class performance_false_positive_diagnostics final : public performance_false_positive_test
  {
  public:
    using performance_false_positive_test::performance_false_positive_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_relative_performance();
  };

  class performance_false_negative_diagnostics final : public performance_false_negative_test
  {
  public:
    using performance_false_negative_test::performance_false_negative_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_relative_performance();
  };
}
