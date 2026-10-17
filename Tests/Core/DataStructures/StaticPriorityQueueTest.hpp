////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "sequoia/TestFramework/RegularTestCore.hpp"

namespace sequoia::testing
{
  class test_static_priority_queue final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    static std::filesystem::path source_file();

    void run_tests();
  private:

    void check_moves_do_not_copy();
    void check_top_and_pop_in_constant_evaluation();
    void check_depth_0();
    void check_depth_1();
    void check_depth_2();
    void check_depth_3();
    void check_construction_from_list_shorter_than_depth();

    constexpr static auto make_static_priority_queue_3();
    constexpr static auto successive_tops_from_list_shorter_than_depth();
  };
}
