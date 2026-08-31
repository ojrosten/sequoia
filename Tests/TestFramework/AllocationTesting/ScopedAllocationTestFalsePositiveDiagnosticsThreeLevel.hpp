////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/PlatformSpecific/Macros.hpp"
#include "sequoia/TestFramework/Macros.hpp"

import std;
import sequoia.test_framework;

/** \file */

namespace sequoia::testing
{
  class scoped_allocation_false_positive_diagnostics_three_level final
    : public regular_allocation_false_positive_test
  {
  public:
    using regular_allocation_false_positive_test::regular_allocation_false_positive_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();

    void run_tests();
  private:
    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_three_level_scoped();
  };
}
