////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class concepts_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept;

    void run_tests();
  private:

    void test_is_range();

    void test_is_allocator();

    void test_is_serializable();

    void test_deep_equality_comparable();

    void test_initializable_from();
  };
}
