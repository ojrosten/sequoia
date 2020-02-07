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
  class test_node_storage final : public unit_test
  {
  public:
    using unit_test::unit_test;

    [[nodiscard]]
    std::string_view source_file_name() const noexcept final;
  private:
    template<class Test>
    friend void do_allocation_tests(Test&);

    using unit_test::check_equality;
      
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
