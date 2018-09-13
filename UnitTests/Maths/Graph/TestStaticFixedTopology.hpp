#pragma once

#include "TestGraphHelper.hpp"

namespace sequoia
{
  namespace unit_testing
  {    
    class test_static_fixed_topology : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

    private:
      void run_tests() override;

      template<class EdgeWeight, class NodeWeight> void test_undirected();
      template<class EdgeWeight, class NodeWeight> void test_embedded_undirected();
      template<class EdgeWeight, class NodeWeight> void test_directed();
      template<class EdgeWeight, class NodeWeight> void test_embedded_directed();
    };
  }
}
