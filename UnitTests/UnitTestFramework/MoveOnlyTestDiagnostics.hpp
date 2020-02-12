////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia:: unit_testing
{
  class move_only_false_positive_diagnostics final : public false_positive_test
  {
  public:
    using false_positive_test::false_positive_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void test_regular_semantics();
  };

  class move_only_false_negative_diagnostics final : public false_negative_test
  {
  public:
    using false_negative_test::false_negative_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void test_regular_semantics();
  };
}
