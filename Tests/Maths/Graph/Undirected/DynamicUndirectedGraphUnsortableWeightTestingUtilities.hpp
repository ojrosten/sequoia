////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "DynamicUndirectedGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace undirected_graph{
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum unsortable_weight_graph_description : std::size_t {
      // x
      nodew = graph_description::end,

      //  /\
      //  \/
      //  x
      node_0w,

      //  /\ /\
      //  \/ \/
      //    x
      node_0w_0w,

      //  /\ /\
      //  \/ \/
      //    x
      node_0_0w,

      //  /\ /\
      //  \/ \/
      //    x
      node_0w_0,

      //  x    x
      node_nodew,

      //  x    x
      nodew_node,

      //  x ---- x
      node_1_nodew_0,

      //  x ---- x
      nodew_1_node_0,

      //   /\
      //   \/
      //   x --- x
      node_0w_1_node_0,

      //         /\
      //         \/
      //   x --- x
      node_1_node_0_1w,

      //  x ==== x
      node_1_1w_node_0_0w,

      //  x ==== x
      node_1w_1w_node_0w_0w,

      //    ----
      //  x ==== x
      node_1_1w_1x_node_0_0w_0x,

      // /-\
      // \ /
      //  x ==== x
      //    ----
      node_0y_1_1w_1x_node_0_0w_0x,
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  class dynamic_undirected_graph_unsortable_weight_operations
  {
    template<maths::network>
    friend struct graph_initialization_checker;
   public:
    using graph_t            = maths::graph<maths::directed_flavour::undirected, EdgeWeight, NodeWeight, EdgeStorageTraits, NodeWeightStorageTraits>;
    using edge_t             = typename graph_t::edge_init_type;
    using node_weight_type   = typename graph_t::node_weight_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    constexpr static bool has_shared_weight{EdgeStorageTraits::edge_sharing == maths::edge_sharing_preference::shared_weight};

    static void execute_operations(regular_test& t)
    {
      auto trg{make_weighted_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
            t.check(equality, description, obtained, prediction);
            if(host != target) t.check_semantics(description, prediction, parent);
          }
      };

      transition_checker<graph_t>::check(report_line(""), trg, checker);
    }

    [[nodiscard]]
    static graph_t make_and_check(regular_test& t, std::string_view description, edges_equivalent_t edgeInit, std::initializer_list<node_weight_type> nodeInit)
    {
      return graph_initialization_checker<graph_t>::make_and_check(t, description, edgeInit, nodeInit);
    }

    static void check_initialization_exceptions(regular_test& t)
    {
      using nodes = std::initializer_list<node_weight_type>;

      // One node
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched loop weights"), [](){ return graph_t{{edge_t{0, 1.0, 1.0}, edge_t{0, 1.0, 2.0}}}; });

      // Two nodes
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched weights"), [](){ return graph_t{{edge_t{1, 1.0, 2.0}}, {edge_t{0, 2.0, 1.0}}}; });

      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched edge/node initialization"), [](){ return graph_t{{}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched edge/node initialization"), [](){ return graph_t{{{}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched edge/node initialization"), [](){ return graph_t{{{edge_t{0, 1.0, -1.2}, edge_t{0, 1.0, -1.2}}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched edge/node initialization"), [](){ return graph_t{{{}, {}}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched edge/node initialization"), [](){ return graph_t{{{edge_t{1}}, {edge_t{0}}}, nodes{1.0}}; });
    }

    [[nodiscard]]
    static transition_graph make_weighted_transition_graph(regular_test& t)
    {
      using base_ops = dynamic_undirected_graph_operations<EdgeWeight, NodeWeight, EdgeStorageTraits, NodeWeightStorageTraits>;
      using namespace undirected_graph;

      auto trg{base_ops::make_transition_graph(t)};

      check_initialization_exceptions(t);

      // 'weighted_graph_description::nodew'
      trg.add_node(make_and_check(t, t.report_line(""), {{}}, {{1.0, -1.0}}));

      // 'weighted_graph_description::node_0w'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}}}, {{0.0}}));

      // 'weighted_graph_description::node_0w_0w'
      trg.add_node(make_and_check(t, t.report_line(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 1.0, -1.0}}}, {{0.0}}));

      // 'weighted_graph_description::node_0_0w'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report_line(""), {{{0, 0.0, 0.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}, {0, 1.0, -1.0}}}, {{0.0}})};
          t.check(equality, t.report_line("Canonical ordering of weighted edges"), graph_t{{{{0, 0.0, 0.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}}}, {{0.0}}}, g);
          return g;
        }());
      
      // 'weighted_graph_description::node_0w_0'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report_line(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}}}, {{0.0}})};
          t.check(equality, t.report_line("Canonical ordering of weighted edges"), graph_t{{{{0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}}}, {{0.0}}}, g);
          return g;
        }());

      return trg;
    }
  };


}