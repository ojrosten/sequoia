////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  class experimental_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_flat_parsing();
  };
}
