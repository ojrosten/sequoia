////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

#include "sequoia/Maths/Sequences/MonotonicSequence.hpp"

namespace sequoia::testing
{
  class monotonic_sequence_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::filesystem::path source_file() const;

    void run_tests();
  private:

    template<bool Check>
    constexpr static maths::static_monotonic_sequence<int, 6, std::greater<int>> make_sequence();

    void test_decreasing_sequence();

    void test_static_decreasing_sequence();

    void test_static_increasing_sequence();
  };
}
