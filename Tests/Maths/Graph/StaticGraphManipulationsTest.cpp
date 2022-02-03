////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "StaticGraphManipulationsTest.hpp"
#include "FixedTopologyGenericTests.hpp"

#include "sequoia/Maths/Graph/StaticGraph.hpp"

namespace sequoia::testing
{
  [[nodiscard]]
  std::string_view test_static_fixed_topology::source_file() const noexcept
  {
    return __FILE__;
  }

  void test_static_fixed_topology::run_tests()
  {
    using namespace maths;

    test_undirected<int, null_weight>();
    test_undirected<int, int>();

    test_embedded_undirected<int, null_weight>();
    test_embedded_undirected<int, int>();

    test_directed<int, null_weight>();
    test_directed<int, int>();

    test_embedded_directed<int, null_weight>();
    test_embedded_directed<int, int>();
   }

  template<class EdgeWeight, class NodeWeight>
  void test_static_fixed_topology::test_undirected()
  {
    using namespace maths;

    undirected_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_graph<directed_flavour::undirected, 4, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_4<g_type>();
    }
  }

  template<class EdgeWeight, class NodeWeight>
  void test_static_fixed_topology::test_embedded_undirected()
  {
    using namespace maths;

    e_undirected_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_embedded_graph<directed_flavour::undirected, 2, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_2<g_type>();
    }
  }

  template<class EdgeWeight, class NodeWeight>
  void test_static_fixed_topology::test_directed()
  {
    using namespace maths;

    directed_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_graph<directed_flavour::directed, 10, 3, EdgeWeight, NodeWeight>;
      checker.template check_3_10<g_type>();
    }
  }

  template<class EdgeWeight, class NodeWeight>
  void test_static_fixed_topology::test_embedded_directed()
  {
    using namespace maths;

    e_directed_fixed_topology_checker<test_static_fixed_topology> checker{*this};

    {
      using g_type = static_embedded_graph<directed_flavour::directed, 2, 2, EdgeWeight, NodeWeight>;
      checker.template check_2_2<g_type>();
    }
  }
}
