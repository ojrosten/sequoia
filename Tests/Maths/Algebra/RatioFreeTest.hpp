////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2025.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class ratio_free_test final : public free_test
  {
  public:
    using free_test::free_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:    
    template<std::integral T>
    void test_ratio();

    template<std::floating_point T>
    void test_ratio();

    template<std::integral T, std::floating_point U>
    void test_ratio();

    template<std::integral T>
    void test_ratio_multiply();

    template<std::floating_point T>
    void test_ratio_multiply();

    template<std::integral T, std::floating_point U>
    void test_ratio_multiply();

    template<std::integral T>
    void test_ratio_divide();

    template<std::floating_point T>
    void test_ratio_divide();

    template<std::integral T, std::floating_point U>
    void test_ratio_divide();
  };
}
