////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/PerformanceTestCore.hpp"

namespace sequoia::testing
{
  class experimental_test final : public performance_test
  {
  public:
    using performance_test::performance_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  };
}
