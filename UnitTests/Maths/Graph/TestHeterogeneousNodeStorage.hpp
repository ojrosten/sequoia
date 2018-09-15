#pragma once

#include "UnitTestUtils.hpp"

#include "HeterogeneousNodeStorage.hpp"

namespace sequoia::unit_testing
{
  class test_heterogeneous_node_storage : public unit_test
  {
  public:
    using unit_test::unit_test;

  private:
    using unit_test::check_equality;
    
    void run_tests() override;
    
    template<class... Ts>
    class storage_tester : public maths::graph_impl::heterogeneous_node_storage<Ts...>
    {
    public:
      template<class... Args>
      constexpr explicit storage_tester(Args&&... args)
        : maths::graph_impl::heterogeneous_node_storage<Ts...>(std::forward<Args>(args)...)
      {
      }
    };
    
    constexpr static auto make_storage() -> storage_tester<float, int>;  
  };
}
