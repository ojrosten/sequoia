////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"

#include "sequoia/Core/Ownership/DataPool.hpp"

namespace sequoia::testing
{
  class data_pool_test final : public move_only_test
  {
  public:
    using move_only_test::move_only_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    void run_tests() final;

    void test_pooled();
    void test_multi_pools();
    void test_spawner();

    ownership::data_pool<int> make_int_pool(const int val);
  };
}
