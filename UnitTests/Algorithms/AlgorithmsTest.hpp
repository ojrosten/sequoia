////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  class algorithms_test final : public free_test
  {
  public:
    using free_test::free_test;
    
    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    void run_tests() final;

    void sort_basic_type();
    void sort_protective_wrapper();
    void sort_partial_edge();

    void cluster_basic_type();

    void lower_bound_basic_type();
    void lower_bound_protective_wrapper();
    void lower_bound_partial_edge();

    void upper_bound_basic_type();

    void equal_range_basic_type();

    void equality();

    void test_rotate();
  };
}
