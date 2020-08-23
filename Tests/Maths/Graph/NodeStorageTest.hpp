////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "NodeStorageTestingUtilities.hpp"

namespace sequoia::testing
{
  class node_storage_test final : public regular_test
  {
  public:
    using regular_test::regular_test;

    [[nodiscard]]
    std::string_view source_file() const noexcept final;
  private:
    template<class Test>
    friend void do_allocation_tests(Test&);
      
    void run_tests() final;

    struct null_weight{};

    template<class Sharing>
    void test_dynamic_node_storage();
    
    void test_static_node_storage();

    template<class Sharing, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation_impl();    

    template<bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
    void test_allocation();
  };
}
