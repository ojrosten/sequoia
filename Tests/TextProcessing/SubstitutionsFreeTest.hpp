////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/FreeTestCore.hpp"

namespace sequoia::testing
{
  class substitutions_free_test final : public free_test
  {
  public:
    using free_test::free_test;

  private:
    [[nodiscard]]
    std::string_view source_file() const noexcept final;

    void run_tests() final;

    void test_camel_case();

    void test_snake_case();

    void test_capitalize();

    void test_uncapitalize();

    void test_replace();

    void test_replace_all();
  };
}
