#pragma once

#include "TestGraphHelper.hpp"

namespace sequoia
{
  namespace unit_testing
  {    
    class test_static_graph : public graph_unit_test
    {
    public:
      using graph_unit_test::graph_unit_test;

    private:
      void run_tests() override;

      template<class NodeWeight, class EdgeWeight> void test_generic_undirected();
      template<class NodeWeight, class EdgeWeight> void test_generic_embedded_undirected();
      template<class NodeWeight, class EdgeWeight> void test_generic_directed();
      template<class NodeWeight, class EdgeWeight> void test_generic_embedded_directed();
    };
  }
}
