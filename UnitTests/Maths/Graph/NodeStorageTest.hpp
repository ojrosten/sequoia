////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "NodeStorageTestingUtilities.hpp"

namespace sequoia::unit_testing
{
  class test_node_storage : public unit_test
  {
  public:
    using unit_test::unit_test;

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  private:
    using unit_test::check_equality;
      
    void run_tests() override;

    struct null_weight{};

    template<class Sharing>
    void test_dynamic_node_storage();
    
    void test_static_node_storage();

    template<class Sharing, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation_impl();
  };
}
