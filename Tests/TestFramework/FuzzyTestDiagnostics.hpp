////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "FuzzyTestCore.hpp"

namespace sequoia:: unit_testing
{
  class fuzzy_false_positive_diagnostics final : public fuzzy_false_positive_test
  {
  public:
    using fuzzy_false_positive_test::fuzzy_false_positive_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void basic_tests();
    void range_tests();
    void container_tests();
  };

  class fuzzy_false_negative_diagnostics final : public fuzzy_false_negative_test
  {
  public:
    using fuzzy_false_negative_test::fuzzy_false_negative_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void basic_tests();
    void range_tests();
    void container_tests();
  };
}
