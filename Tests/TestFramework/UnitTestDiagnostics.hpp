////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "RegularTestCore.hpp"

namespace sequoia:: testing
{
  class false_positive_diagnostics final : public false_positive_regular_test
  {
  public:
    using false_positive_regular_test::false_positive_regular_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void basic_tests();
    void test_container_checks();
    void test_mixed();
    void test_regular_semantics();
    void test_equivalence_checks();
    void test_weak_equivalence_checks();
  };

  class false_negative_diagnostics final : public false_negative_regular_test
  {
  public:
    using false_negative_regular_test::false_negative_regular_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void basic_tests();
    void test_container_checks();
    void test_mixed();
    void test_regular_semantics();
    void test_equivalence_checks();
    void test_weak_equivalence_checks();
  };
}
