////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include "DynamicGraphEmbeddedTest.hpp"

namespace sequoia
{
  namespace unit_testing
  {
    void test_edge_insertion::run_tests()
    {
      using namespace maths;

      struct null_weight {};
      
      {
        graph_test_helper<null_weight, null_weight> helper{};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
      }
      
      {
        graph_test_helper<null_weight, int> helper{};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
      }

      {
        graph_test_helper<int, null_weight> helper{};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_weighted_edge_insertions>(*this);
        
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_weighted_edge_insertions>(*this);
      }

      {
        graph_test_helper<int, int> helper{};
        helper.run_individual_test<graph_flavour::undirected_embedded, generic_edge_insertions>(*this);
        helper.run_individual_test<graph_flavour::directed_embedded, generic_edge_insertions>(*this);
      }
    }

    template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      class EdgeWeightPooling,
      class NodeWeightPooling,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    void generic_edge_insertions
    <
      GraphFlavour,      
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
    >::execute_operations()
    {
      using namespace maths;

      using inv_t = inversion_constant<true>;
        
      graph_t g{};
      constexpr bool ThrowOnError{graph_t::connectivity_type::throw_on_range_error};

      if constexpr(ThrowOnError)
      {
        check_exception_thrown<std::out_of_range>(LINE(""), [&g]() { g.insert_join(g.cbegin_edges(0), 0); });
      }


      //   X
      g.add_node();
      check_equality(LINE(""), g, {{}});

      g.insert_join(g.cbegin_edges(0), 0);
      //  /<\
      //  \ /
      //   X
      
      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(LINE(""), g, {{{0,1}, {0,0}}});
      }
      else
      {
        check_equality(LINE(""), g, {{{0,inv_t{},1}, {0,inv_t{},0}}});
        check_regular_semantics(LINE("Regular semantics"), g, {{{0,0,1}, {0,0,0}}});
      }

      check_regular_semantics(LINE("Regular semantics"), g, graph_t{});

      g.insert_join(g.cbegin_edges(0) + 1, 3);
      //   /<\/>\
      //  /  /\  \
      //  \ /  \  \
      //      X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(LINE(""), g, {{{0,2}, {0,3}, {0,0}, {0,1}}});
      }
      else
      {
        check_equality(LINE(""), g, {{{0,inv_t{},2}, {0,0,3}, {0,inv_t{},0}, {0,0,1}}});
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
        check_equality(LINE(""), g, {{{0,1}, {0,0}, {0,4}, {0,5}, {0,2}, {0,3}}});
      }
      else
      {
        check_equality(LINE(""), g, {{{0,inv_t{},1}, {0,inv_t{},0}, {0,inv_t{},4}, {0,0,5}, {0,inv_t{},2}, {0,0,3}}});
      }

      g.insert_node(0);

      //          /<\/>\
      //     /<\ /  /\  \
      //     \ / \ /  \  \
      // X          X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(LINE(""), g, {{}, {{1,1}, {1,0}, {1,4}, {1,5}, {1,2}, {1,3}}});
      }
      else
      {
        check_equality(LINE(""), g, {{}, {{1,inv_t{},1}, {1,inv_t{},0}, {1,inv_t{},4}, {1,1,5}, {1,inv_t{},2}, {1,1,3}}});
      }

      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1));
      //          /<\/>\
      //     /<\ /  /\  \
      //     \ / \ /  \  \
      // X---->-----X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(LINE(""), g, {{{1,0}}, {{0,0}, {1,2}, {1,1}, {1,5}, {1,6}, {1,3}, {1,4}}});
      }
      else
      {
        check_equality(LINE(""), g, {{{0,1,0}}, {{0,1,0}, {1,inv_t{},2}, {1,inv_t{},1}, {1,inv_t{},5}, {1,1,6}, {1,inv_t{},3}, {1,1,4}}});
      }

      g.insert_join(g.cbegin_edges(1)+1, g.cbegin_edges(0));
      //          /<\/>\
      //     /<\ /  /\  \
      //     \ / \ /  \  \
      // X---->\/----
      //  -----/\<---X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(
          LINE(""),
          g,
          {{{1,1}, {1,0}}, {{0,1}, {0,0}, {1,3}, {1,2}, {1,6}, {1,7}, {1,4}, {1,5}}});
      }
      else
      {
        check_equality(
          LINE(""),
          g,
          {{{1,0,1}, {0,1,0}}, {{0,1,1}, {1,0,0}, {1,inv_t{},3}, {1,inv_t{},2}, {1,inv_t{},6}, {1,1,7}, {1,inv_t{},4}, {1,1,5}}});
      }
    }

     template
    <
      maths::graph_flavour GraphFlavour,      
      class EdgeWeight,
      class NodeWeight,      
      class EdgeWeightPooling,
      class NodeWeightPooling,
      class EdgeStorageTraits,
      class NodeWeightStorageTraits
    >
    void generic_weighted_edge_insertions
    <
      GraphFlavour,      
      EdgeWeight,
      NodeWeight,
      EdgeWeightPooling,
      NodeWeightPooling,
      EdgeStorageTraits,
      NodeWeightStorageTraits
    >::execute_operations()
    {
      using namespace maths;
        
      graph_t g{};

      g.add_node();
      g.add_node();

      g.insert_join(g.cbegin_edges(0), g.cbegin_edges(1), 5);
      // X----5----X

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(LINE(""), g, {{{1,0,5}}, {{0,0,5}}});
      }
      else
      {
        check_equality(LINE(""), g, {{{0,1,0,5}}, {{0,1,0,5}}});
      }

      g.insert_join(g.cend_edges(1), 0, 6);
      //
      //           6
      //          /<\
      // X----5---\X/

      if constexpr(GraphFlavour == graph_flavour::undirected_embedded)
      {
        check_equality(LINE(""), g, {{{1,1,5}}, {{1,2,6}, {0,0,5}, {1,0,6}}});
        check_regular_semantics(LINE("Regular semantics"), g, {{{1,0,5}}, {{0,0,5}, {1,2,6}, {1,1,6}}});
      }
      else
      {
        check_equality(LINE(""), g, {{{0,1,1,5}}, {{1,inversion_constant<true>{},2,6}, {0,1,0,5}, {1,inversion_constant<true>{},0,6}}});
        check_regular_semantics(LINE("Regular semantics"), g, {{{0,1,1,5}}, {{1,1,2,6}, {0,1,0,5}, {1,1,0,6}}});
      }

      check_regular_semantics(LINE("Regular semantics"), g, graph_t{});
    }
  }
}
