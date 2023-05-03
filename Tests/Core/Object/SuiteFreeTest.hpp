////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class suite_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept;

    void run_tests();
  private:
    void test_flat_suite();
    void test_nested_suite();
    void test_name_filter();

    void test_flat_to_tree();
    void test_nested_to_tree();
  };
}
