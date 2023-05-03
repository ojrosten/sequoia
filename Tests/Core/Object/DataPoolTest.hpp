////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "sequoia/TestFramework/MoveOnlyTestCore.hpp"

#include "sequoia/Core/Object/DataPool.hpp"

namespace sequoia::testing
{
  class data_pool_test final : public move_only_test
  {
  public:
    using move_only_test::move_only_test;

    [[nodiscard]]
    std::filesystem::path source_file() const noexcept;

    void run_tests();
  private:

    void test_pooled();
    void test_multi_pools();
    void test_faithful_producer();

    object::data_pool<int> make_int_pool(const int val);
  };
}
