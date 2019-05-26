////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "StaticGraphTest.hpp"
#include "GraphInitializationTestingUtilities.hpp"

#include "StaticGraph.hpp"

namespace sequoia::unit_testing
{  
  void test_static_graph::run_tests()
  {
    test_generic_undirected<null_weight,null_weight>();
    test_generic_undirected<null_weight, int>();
    test_generic_undirected<null_weight,unsortable>();
    test_generic_undirected<null_weight, big_unsortable>();
    test_generic_undirected<int, null_weight>();
    test_generic_undirected<int, int>();
    test_generic_undirected<int, unsortable>();
    test_generic_undirected<int, big_unsortable>();

    test_generic_embedded_undirected<null_weight, null_weight>();
    test_generic_embedded_undirected<null_weight, int>();
    test_generic_embedded_undirected<null_weight, unsortable>();
    test_generic_embedded_undirected<null_weight, big_unsortable>();
    test_generic_embedded_undirected<int, null_weight>();
    test_generic_embedded_undirected<int, int>();
    test_generic_embedded_undirected<int, unsortable>();
    test_generic_embedded_undirected<int, big_unsortable>();
    
    test_generic_directed<null_weight, null_weight>();
    test_generic_directed<null_weight, int>();
    test_generic_directed<null_weight, unsortable>();
    test_generic_directed<null_weight, big_unsortable>();
    test_generic_directed<int, null_weight>();
    test_generic_directed<int, int>();
    test_generic_directed<int, unsortable>();
    test_generic_directed<int, big_unsortable>();
    
    test_generic_embedded_directed<null_weight, null_weight>();
    test_generic_embedded_directed<null_weight, int>();
    test_generic_embedded_directed<null_weight, unsortable>();
    test_generic_embedded_directed<null_weight, big_unsortable>();
    test_generic_embedded_directed<int, null_weight>();
    test_generic_embedded_directed<int, int>();
    test_generic_embedded_directed<int, unsortable>();
    test_generic_embedded_directed<int, big_unsortable>();
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_graph::test_generic_undirected()
  {
    using namespace maths;

    undirected_init_checker<test_static_graph> checker{*this};
    
    {
      using g_type = static_graph<directed_flavour::undirected, 0, 0, EdgeWeight, NodeWeight>;
      checker.template check_0_0<g_type>();
    }

    {
      using g_type = static_graph<directed_flavour::undirected, 0, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }
    
    {
      using g_type = static_graph<directed_flavour::undirected, 1, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_1<g_type>();
    }

    {
      using g_type = static_graph<directed_flavour::undirected, 2, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_2<g_type>();
    }

    {
      using g_type = static_graph<directed_flavour::undirected, 0, 2, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }

    {
      using g_type = static_graph<directed_flavour::undirected, 1, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_1<g_type>();

      // Think about alignment for non-empty T...
      if constexpr (std::is_empty_v<NodeWeight> && std::is_empty_v<EdgeWeight>)
        check_equality(4*sizeof(char), sizeof(g_type), LINE("2 bytes for each half edge and 2 for the partition data"));

      static_assert(std::is_same_v<typename g_type::edge_index_type, unsigned char>);
    }
    
    {
      using g_type = static_graph<directed_flavour::undirected, 2, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_2<g_type>();
    }

    {
      using g_type = static_graph<directed_flavour::undirected, 3, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_3<g_type>();
    }

    {
      using g_type = static_graph<directed_flavour::undirected, 4, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_4<g_type>();
    }
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_graph::test_generic_embedded_undirected()
  {
    using namespace maths;

    undirected_embedded_init_checker<test_static_graph> checker{*this};
    
    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 0, 0, EdgeWeight, NodeWeight>;
      checker.template check_0_0<g_type>();
    }

    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 0, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }
    
    {      
      using g_type = static_embedded_graph<directed_flavour::undirected, 1, 1, EdgeWeight, NodeWeight>;      
      checker.template check_1_1<g_type>();
    }

    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 2, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_2<g_type>();      
    }

    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 0, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_0<g_type>();
    }
    
    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 1, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_1<g_type>();
    }

    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 2, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_2<g_type>();
    }
    
    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 3, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_3<g_type>();
    }
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_graph::test_generic_directed()
  {
     using namespace maths;

     directed_init_checker<test_static_graph> checker{*this};

     {
       using g_type = static_graph<directed_flavour::directed, 0, 0, EdgeWeight, NodeWeight>;
       checker.template check_0_0<g_type>();
     }

     {
       using g_type = static_graph<directed_flavour::directed, 0, 1, EdgeWeight, NodeWeight>;
       checker.template check_1_0<g_type>();
     }
     
     {
       using g_type = static_graph<directed_flavour::directed, 1, 1, EdgeWeight, NodeWeight>;
       checker.template check_1_1<g_type>();
     }

     {
       using g_type = static_graph<directed_flavour::directed, 2, 1, EdgeWeight, NodeWeight>;
       checker.template check_1_2<g_type>();
     }

     {
       using g_type = static_graph<directed_flavour::undirected, 0, 2, EdgeWeight, NodeWeight>;
       checker.template check_2_0<g_type>();
     }
     
     {
       using g_type = static_graph<directed_flavour::directed, 1, 2, EdgeWeight, NodeWeight>;
       checker.template check_2_1<g_type>();
     }

     {
       using g_type = static_graph<directed_flavour::directed, 2, 3, EdgeWeight, NodeWeight>;
       checker.template check_3_2<g_type>();
     }

     {
       using g_type = static_graph<directed_flavour::directed, 2, 4, EdgeWeight, NodeWeight>;
       checker.template check_4_2<g_type>();
     }
  }

  template<class NodeWeight, class EdgeWeight>
  void test_static_graph::test_generic_embedded_directed()
  {
    using namespace maths;

    directed_embedded_init_checker<test_static_graph> checker{*this};

    {
      using g_type = static_embedded_graph<directed_flavour::directed, 0, 0, EdgeWeight, NodeWeight>;
      checker.template check_0_0<g_type>();
    }

    {
      using g_type = static_embedded_graph<directed_flavour::directed, 0, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_0<g_type>();
    }
    
    {
      using g_type = static_embedded_graph<directed_flavour::directed, 1, 1, EdgeWeight, NodeWeight>;
      checker.template check_1_1<g_type>();
    }

    {
      using g_type = static_embedded_graph<directed_flavour::directed, 0, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_0<g_type>();
    }
    
    {
      using g_type = static_embedded_graph<directed_flavour::directed, 1, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_1<g_type>();
    }
    
    {
      using g_type = static_embedded_graph<directed_flavour::directed, 1, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_1<g_type>();
    }
  }
}
