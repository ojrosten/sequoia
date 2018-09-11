#include "TestEdgeInsertion.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_edge_insertion::run_tests()
    {
      using std::complex;
      using namespace maths;

      struct null_weight {};
      
      {
        graph_test_helper<null_weight, null_weight> helper{"Unweighted"};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
      }
      
      {
        graph_test_helper<int, null_weight> helper{"int, Unweighted"};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
      }

      {
        graph_test_helper<null_weight, int> helper{"Unweighted, int"};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_weighted_edge_insertions>(*this);
        
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_weighted_edge_insertions>(*this);
      }

      {
        graph_test_helper<int, int> helper{"int, int"};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
      }
    }

    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, template<class> class> class EdgeStorageTraits
    >
    void generic_edge_insertions
    <
      GraphFlavour,
      NodeWeight,
      EdgeWeight,
      NodeWeightStorage,
      EdgeWeightStorage,
      EdgeStorageTraits
    >::execute_operations()
    {
      using namespace maths;

      using edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;
        
      graph_t g{};
      constexpr bool ThrowOnError{graph_t::throw_on_range_error};

      if constexpr(ThrowOnError)
      {
        check_exception_thrown<std::out_of_range>([&g]() { g.insert_join(g.cbegin_edges(0), 0); }, LINE(""));
      }


      //   X
      g.add_node();
      check_graph(g, {{}}, {{}}, LINE(""));

      g.insert_join(g.cbegin_edges(0), 0);
      //  /<\
      //  \ /
      //   X
      
      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(g, {{edge{0,0,1}, edge{0,0,0}}}, {{}}, LINE(""));
      }
      else
      {
        check_graph(g, {{edge{0,inverted_constant<true>{},1}, edge{0,inverted_constant<true>{},0}}}, {{}}, LINE(""));
      }

      g.insert_join(g.cbegin_edges(0) + 1, 3);
      //   /<\/>\
      //  /  /\  \
      //  \ /  \  \
      //      X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(g, {{edge{0,0,2}, edge{0,0,3}, edge{0,0,0}, edge{0,0,1}}}, {{}}, LINE(""));
      }
      else
      {
        check_graph(g, {{edge{0,inverted_constant<true>{},2}, edge{0,0,3}, edge{0,inverted_constant<true>{},0}, edge{0,0,1}}}, {{}}, LINE(""));
      }

      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(0));
      // For loops created by a pair of iterators, the second
      // insertion point is defined to be distance(cbegin_edges(n), iter),
      // which in this case is 0.
      
      //      /<\/>\
      // /<\ /  /\  \
      // \ / \ /  \  \
      //        X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(
          g,
          {{edge{0,0,1}, edge{0,0,0}, edge{0,0,4}, edge{0,0,5}, edge{0,0,2}, edge{0,0,3}}}, {{}},
          LINE(""));
      }
      else
      {
        check_graph(
          g,
          {{edge{0,inverted_constant<true>{},1}, edge{0,inverted_constant<true>{},0}, edge{0,inverted_constant<true>{},4}, edge{0,0,5}, edge{0,inverted_constant<true>{},2}, edge{0,0,3}}}, {{}},
          LINE(""));
      }

      g.insert_node(0);

      //          /<\/>\
      //     /<\ /  /\  \
      //     \ / \ /  \  \
      // X          X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(
          g,
          {{}, {edge{1,1,1}, edge{1,1,0}, edge{1,1,4}, edge{1,1,5}, edge{1,1,2}, edge{1,1,3}}}, {{}, {}},
          LINE(""));
      }
      else
      {
        check_graph(
          g,
          {{}, {edge{1,inverted_constant<true>{},1}, edge{1,inverted_constant<true>{},0}, edge{1,inverted_constant<true>{},4}, edge{1,1,5}, edge{1,inverted_constant<true>{},2}, edge{1,1,3}}}, {{}, {}},
          LINE(""));
      }

      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1));
      //          /<\/>\
      //     /<\ /  /\  \
      //     \ / \ /  \  \
      // X---->-----X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(
          g,
          {{edge{0,1,0}}, {edge{0,1,0}, edge{1,1,2}, edge{1,1,1}, edge{1,1,5}, edge{1,1,6}, edge{1,1,3}, edge{1,1,4}}}, {{}, {}},
          LINE(""));
      }
      else
      {
        check_graph(
          g,
          {{edge{0,1,0}}, {edge{0,1,0}, edge{1,inverted_constant<true>{},2}, edge{1,inverted_constant<true>{},1}, edge{1,inverted_constant<true>{},5}, edge{1,1,6}, edge{1,inverted_constant<true>{},3}, edge{1,1,4}}}, {{}, {}},
          LINE(""));
      }

      g.insert_join(g.cbegin_edges(1)+1, g.cbegin_edges(0));
      //          /<\/>\
      //     /<\ /  /\  \
      //     \ / \ /  \  \
      // X---->\/----
      //  -----/\<---X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(
          g,
          {{edge{0,1,1}, edge{0,1,0}}, {edge{0,1,1}, edge{0,1,0}, edge{1,1,3}, edge{1,1,2}, edge{1,1,6}, edge{1,1,7}, edge{1,1,4}, edge{1,1,5}}}, {{}, {}},
          LINE(""));
      }
      else
      {
        check_graph(
          g,
          {{edge{1,0,1}, edge{0,1,0}}, {edge{0,1,1}, edge{1,0,0}, edge{1,inverted_constant<true>{},3}, edge{1,inverted_constant<true>{},2}, edge{1,inverted_constant<true>{},6}, edge{1,1,7}, edge{1,inverted_constant<true>{},4}, edge{1,1,5}}}, {{}, {}},
          LINE(""));
      }
    }

    template
    <
      maths::graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      template <class> class NodeWeightStorage,
      template <class> class EdgeWeightStorage,
      template <class, template<class> class> class EdgeStorageTraits
    >
    void generic_weighted_edge_insertions
    <
      GraphFlavour,
      NodeWeight,
      EdgeWeight,
      NodeWeightStorage,
      EdgeWeightStorage,
      EdgeStorageTraits
    >::execute_operations()
    {
      using namespace maths;

      using edge = embedded_edge<EdgeWeight, data_sharing::independent, utilities::protective_wrapper<EdgeWeight>>;
        
      graph_t g{};

      g.add_node();
      g.add_node();

      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), 5);
      // X----5----X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(g, {{edge{0,1,0,5}}, {edge{0,1,0,5}}}, {{}, {}}, LINE(""));
      }
      else
      {
        check_graph(g, {{edge{0,1,0,5}}, {edge{0,1,0,5}}}, {{}, {}}, LINE(""));
      }

      g.insert_join(g.cend_edges(1), 0, 6);
      //
      //           6
      //          /<\
      // X----5---\X/

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_graph(g, {{edge{0,1,1,5}}, {edge{1,1,2,6}, edge{0,1,0,5}, edge{1,1,0,6}}}, {{}, {}}, LINE(""));
      }
      else
      {
        check_graph(g, {{edge{0,1,1,5}}, {edge{1,inverted_constant<true>{},2,6}, edge{0,1,0,5}, edge{1,inverted_constant<true>{},0,6}}}, {{}, {}}, LINE(""));
      }
    }
  }
}
