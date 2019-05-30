////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphUpdateTest.hpp"

namespace sequoia::unit_testing
{
  void test_graph_update::run_tests()
  {
    graph_test_helper<std::vector<double>, std::vector<int>> helper;
    helper.run_tests<test_bf_update>(*this);

    graph_test_helper<size_t, size_t> helper2;
    helper2.run_tests<test_update>(*this);
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_bf_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::execute_operations()
  {
    graph_t graph;
    graph.add_node();
    graph.add_node();
    graph.add_node();

    graph.join(0, 1);
    graph.join(1, 0);
    graph.join(1, 2);
    graph.join(2, 0);
    graph.join(2, 2);
    graph.join(2, 2);

    //    
    // /\    /\
    // --  0 --
    //    / \
    //   /   \
    //  0=====0

    auto nodeFn1 = [&graph](const std::size_t index) { graph.node_weight(graph.cbegin_node_weights() + index, std::vector<int>{static_cast<int>(index)}); };
    maths::breadth_first_search(graph, false, 0, nodeFn1);

    check_equality(std::vector<int>{0}, *graph.cbegin_node_weights(), "Node 0 has a vector holding a single zero");
    check_equality(std::vector<int>{1}, *(graph.cbegin_node_weights()+1), "Node 1 has a vector holding a single one");
    check_equality(std::vector<int>{2}, *(graph.cbegin_node_weights()+2), "Node 2 has a vector holding a single two");

    auto nodeFn2 = [&graph](const std::size_t index) { graph.node_weight(graph.cbegin_node_weights() + index, std::vector<int>{3 - static_cast<int>(index)}); };
    maths::breadth_first_search(graph, false, 0, maths::null_functor(), nodeFn2);

    check_equality(std::vector<int>{3}, *graph.cbegin_node_weights(), "Node 0 has a vector holding a single three");
    check_equality(std::vector<int>{2}, *(graph.cbegin_node_weights()+1), "Node 1 has a vector holding a single two");
    check_equality(std::vector<int>{1}, *(graph.cbegin_node_weights()+2), "Node 2 has a vector holding a single one");

    auto edgeFn1 = [&graph](auto edgeIter) {
      const std::size_t node{edgeIter.partition_index()};
      const std::size_t index{static_cast<std::size_t>(distance(graph.cbegin_edges(node), edgeIter))};
      std::tuple<std::size_t, std::size_t, std::size_t> nthConnInds(nth_connection_indices(graph, node, index));
      const std::size_t nthConn = std::get<2>(nthConnInds);
      graph.set_edge_weight(edgeIter, std::vector<double>{static_cast<double>(nthConn)});
    };

    maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), edgeFn1);

    check_equality(std::vector<double>{0}, get_edge(graph, 0, 1, 0).weight(), "Zeroth connection from node 0 --> 1 has vector holding single zero");
    undirected(GraphFlavour) ? check_equality(std::vector<double>{1}, get_edge(graph, 0, 1, 1).weight(), "First connection from node 0 --> 1 has vector holding single one")
      : check_exception_thrown<std::out_of_range>([&graph](){ return get_edge(graph, 0, 1, 1).weight(); }, "Only one connection from node 0 --> 1");

    check_equality(std::vector<double>{0}, get_edge(graph, 1, 0, 0).weight(), "Zeroth connection from node 1 --> 0 has vector holding single zero");
    undirected(GraphFlavour) ? check_equality(std::vector<double>{1}, get_edge(graph, 1, 0, 1).weight(), "First connection from node 1 --> 0 has vector holding single one")
      : check_exception_thrown<std::out_of_range>([&graph](){ return get_edge(graph, 1, 0, 1).weight(); }, "Only one connection from node 1 --> 0");

    check_equality(std::vector<double>{0}, get_edge(graph, 1, 2, 0).weight(), "Zeroth connection from node 1 --> 2 has vector holding single zero");
    undirected(GraphFlavour) ? check_equality(std::vector<double>{0}, get_edge(graph, 2, 1, 0).weight(), "Zeroth connection from node 2 --> 1 has vector holding single zero")
      : check_exception_thrown<std::out_of_range>([&graph]() { return get_edge(graph, 2, 1, 0).weight(); }, "No connections from node 2 --> 1");

    check_equality(std::vector<double>{0}, get_edge(graph, 2, 0, 0).weight(), "Zeroth connection from node 2 --> 0 has vector holding single zero");
    undirected(GraphFlavour) ? check_equality(std::vector<double>{0}, get_edge(graph, 0, 2, 0).weight(), "Zeroth connection from node 0 --> 2 has vector holding single zero")
      : check_exception_thrown<std::out_of_range>([&graph]() { return get_edge(graph, 0, 2, 0).weight(); }, "No connections from node 0 --> 2");

    check_equality(std::vector<double>{0}, get_edge(graph, 2, 2, 0).weight(), "Zeroth connection from node 2 --> 2 has vector holding single zero");
    undirected(GraphFlavour) ? check_equality(std::vector<double>{0}, get_edge(graph, 2, 2, 1).weight(), "First connection from node 2 --> 2 has vector holding single zero"),
      check_equality(std::vector<double>{2}, get_edge(graph, 2, 2, 2).weight(), "Second connection from node 2 --> 2 has vector holding single two"),
      check_equality(std::vector<double>{2}, get_edge(graph, 2, 2, 3).weight(), "Second connection from node 2 --> 2 has vector holding single two")
      : check_equality(std::vector<double>{1}, get_edge(graph, 2, 2, 1).weight(), "First connection from node 2 --> 2 has vector holding single one");

    if constexpr(UndirectedType::value)
    {
      test_second_edge_traversal_update(graph);
    }
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,      
    template <class> class EdgeWeightPooling,
    template <class> class NodeWeightPooling,
    template <maths::graph_flavour, class, template<class> class> class EdgeStorageTraits,
    template <class, template<class> class, bool> class NodeWeightStorageTraits
  >
  void test_bf_update<
    GraphFlavour,
    EdgeWeight,
    NodeWeight,
    EdgeWeightPooling,
    NodeWeightPooling,
    EdgeStorageTraits,
    NodeWeightStorageTraits>::test_second_edge_traversal_update(graph_t& graph)
  {
    auto edgeFn2 = [&graph](auto edgeIter) {
      const std::size_t node{edgeIter.partition_index()};
      const std::size_t index{static_cast<std::size_t>(distance(graph.cbegin_edges(node), edgeIter))};
      std::tuple<std::size_t, std::size_t, std::size_t> nthConnInds(nth_connection_indices(graph, node, index));
      const std::size_t target = std::get<1>(nthConnInds);
      const std::size_t nthConn = std::get<2>(nthConnInds);
      const double val1{static_cast<double>(target)},
      val2{static_cast<double>(nthConn) + 1};
      graph.set_edge_weight(edgeIter, std::vector<double>{val1, val2});
    };

    maths::breadth_first_search(graph, false, 0, maths::null_functor(), maths::null_functor(), maths::null_functor(), edgeFn2);

    check_equality(std::vector<double>{0, 1}, get_edge(graph, 0, 1, 0).weight(), "Zeroth connection from node 0 --> 1 has vector holding (0,1)");
    check_equality(std::vector<double>{0, 2}, get_edge(graph, 0, 1, 1).weight(), "First connection from node 0 --> 1 has vector holding (0,2)");
    check_equality(std::vector<double>{0, 1}, get_edge(graph, 0, 2, 0).weight(), "Zeroth connection from node 0 --> 2 has vector holding (0,1)");

    check_equality(std::vector<double>{0, 1}, get_edge(graph, 1, 0, 0).weight(), "Zeroth connection from node 1 --> 0 has vector holding (0,1)");
    check_equality(std::vector<double>{0, 2}, get_edge(graph, 1, 0, 1).weight(), "First connection from node 1 --> 0 has vector holding (0,2)");
    check_equality(std::vector<double>{1, 1}, get_edge(graph, 1, 2, 0).weight(), "Zeroth connection from node 1 --> 2 has vector holding (1,1)");

    check_equality(std::vector<double>{0, 1}, get_edge(graph, 2, 0, 0).weight(), "Zeroth connection from node 2 --> 0 has vector holding (0,1)");
    check_equality(std::vector<double>{1, 1}, get_edge(graph, 2, 1, 0).weight(), "Zeroth connection from node 2 --> 1 has vector holding (1,1)");
    undirected(GraphFlavour) ? check_equality(std::vector<double>{2,2}, get_edge(graph, 2, 2, 0).weight(), "Zeroth connection from node 2 --> 2 has vector holding (2,2)")
      : check_equality(std::vector<double>{0}, get_edge(graph, 2, 2, 0).weight(), "Zeroth connection from node 2 --> 2 isn't traversed twice, so keeps value from first traversal update");

    undirected(GraphFlavour) ? check_equality(std::vector<double>{2,2}, get_edge(graph, 2, 2, 1).weight(), "First connection from node 2 --> 2 has vector holding (2,2)"),
      check_equality(std::vector<double>{2,4}, get_edge(graph, 2, 2, 2).weight(), "Second connection from node 2 --> 2 has vector holding (2,4)"),
      check_equality(std::vector<double>{2,4}, get_edge(graph, 2, 2, 2).weight(), "Third connection from node 2 --> 2 has vector holding (2,4)")
      : check_equality(std::vector<double>{1}, get_edge(graph, 2, 2, 1).weight(), "First connection from node 2 --> 2 isn't traversed twice, so keeps value from first traversal update"); 
  }
}
