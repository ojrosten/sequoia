#include "TestStaticFixedTopology.hpp"
#include "TestFixedTopologyHelper.hpp"

namespace sequoia::unit_testing
{
  void test_static_fixed_topology::run_tests()
  {
    test_undirected<null_weight, int>();
    test_undirected<int, int>();
    
    test_embedded_undirected<null_weight, int>();
    test_embedded_undirected<int, int>();
    
    test_directed<null_weight, int>();
    test_directed<int, int>();
    
    test_embedded_directed<null_weight, int>();
    test_embedded_directed<int, int>();    
   }

  template<class NodeWeight, class EdgeWeight>
  void test_static_fixed_topology::test_undirected()
  {
    using namespace maths;
    
    undirected_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_graph<directed_flavour::undirected, 2, 4, NodeWeight, EdgeWeight>;
      checker.template check_2_4<g_type>();
    }
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_fixed_topology::test_embedded_undirected()
  {
    using namespace maths;
    
    e_undirected_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 2, 2, NodeWeight, EdgeWeight>;
      checker.template check_2_2<g_type>();
    }
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_fixed_topology::test_directed()
  {
    using namespace maths;
    
    directed_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_graph<directed_flavour::directed, 3, 10, NodeWeight, EdgeWeight>;
      checker.template check_3_10<g_type>();
    }
  }
  
  template<class NodeWeight, class EdgeWeight>
  void test_static_fixed_topology::test_embedded_directed()
  {
    using namespace maths;
    
    e_directed_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_embedded_graph<directed_flavour::directed, 2, 2, NodeWeight, EdgeWeight>;
      checker.template check_2_2<g_type>();
    }
  }
}
