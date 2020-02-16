////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

#include "MonotonicSequence.hpp"

namespace sequoia::unit_testing
{
  class monotonic_sequence_test final : public unit_test
  {
  public:
    using unit_test::unit_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:    
    void run_tests() final;

    template<bool Check>
    constexpr static maths::static_monotonic_sequence<int, 6, std::greater<int>> make_sequence();
    
    void test_decreasing_sequence();

    void test_static_decreasing_sequence();

    void test_static_increasing_sequence();
  };
}
