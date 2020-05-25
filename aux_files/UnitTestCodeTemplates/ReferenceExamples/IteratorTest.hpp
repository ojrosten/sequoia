////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestUtils.hpp"

namespace sequoia::testing
{
  class iterator_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;
  };
}
