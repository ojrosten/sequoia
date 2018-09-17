#pragma once

#include "GraphTestingUtils.hpp"

namespace sequoia::unit_testing
{    
  class test_heterogeneous_static_graph : public graph_unit_test
  {
  public:
    using graph_unit_test::graph_unit_test;

  private:
    void run_tests() override;
    
    constexpr static auto make_undirected_graph();
    constexpr static auto make_directed_graph();

    void test_generic_undirected();
    void test_generic_embedded_undirected();
    void test_generic_directed();
    void test_generic_embedded_directed();
  };
}
