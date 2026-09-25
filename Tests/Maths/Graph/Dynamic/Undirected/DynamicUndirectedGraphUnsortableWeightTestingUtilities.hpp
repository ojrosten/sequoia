////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "DynamicUndirectedGraphTestingUtilities.hpp"

namespace sequoia::testing
{
  namespace undirected_graph{
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum unsortable_weight_graph_description : std::size_t {
      // x
      us_nodew = graph_description::end,

      //  /\
      //  \/
      //  x
      us_node_0w,

      //  /\ /\
      //  \/ \/
      //    x
      us_node_0w_0w,

      //  /\ /\
      //  \/ \/
      //    x
      us_node_0_0w,

      //  /\ /\
      //  \/ \/
      //    x
      us_node_0w_0,

      //   /\
      //   \/
      //   x --- x
      us_node_0w_1_node_0,

      //   /\
      //   \/
      //   x --- x
      us_node_0_1w_node_0w,

      //         /\
      //         \/
      //   x --- x
      us_node_1_node_1w_0,

      //  x ==== x
      us_node_1_1w_node_0_0w,

      //  x ==== x
      us_node_1w_1_node_0_0w,

      //    ====
      //  x ==== x
      us_node_1_1_1w_1w_node_0w_0w_0_0,

      // /-\
      // \ /
      //  x ==== x
      //    ====
      us_node_0w_1_1_1w_1w_node_0w_0w_0_0
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class dynamic_undirected_graph_unsortable_weight_operations
  {
   public:
    using graph_type            = maths::undirected_graph<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
    using edge_init_type        = graph_type::edge_init_type;
    using node_weight_type      = graph_type::node_weight_type;
    using edges_equivalent_type = std::initializer_list<std::initializer_list<edge_init_type>>;
    using transition_graph      = transition_checker<graph_type>::transition_graph;

    static void execute_operations(regular_test& t)
    {
      auto trg{make_weighted_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const graph_type& obtained, const graph_type& prediction, const graph_type& parent, std::size_t host, std::size_t target) {
            t.check(equality, {description, no_source_location}, obtained, prediction);
            if(host != target) t.check_semantics({description, no_source_location}, prediction, parent);
          }
      };

      transition_checker<graph_type>::check(t.report(""), trg, checker);
    }

    [[nodiscard]]
    static graph_type make_and_check(regular_test& t, std::string_view description, edges_equivalent_type edgeInit, std::initializer_list<node_weight_type> nodeInit)
    {
      return graph_initialization_checker<graph_type>::make_and_check(t, description, edgeInit, nodeInit);
    }

    static void check_initialization_exceptions(regular_test& t)
    {
      using nodes = std::initializer_list<node_weight_type>;

      // One node
      t.check_exception_thrown<std::logic_error>("Mismatched loop weights", [](){ return graph_type{{edge_init_type{0, 1.0, 1.0}, edge_init_type{0, 1.0, 2.0}}}; });

      // Two nodes
      t.check_exception_thrown<std::logic_error>("Mismatched weights", [](){ return graph_type{{edge_init_type{1, 1.0, 2.0}}, {edge_init_type{0, 2.0, 1.0}}}; });

      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_type{{}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_type{{{}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_type{{{edge_init_type{0, 1.0, -1.2}, edge_init_type{0, 1.0, -1.2}}}, nodes{1.0, 2.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_type{{{}, {}}, nodes{1.0}}; });
      t.check_exception_thrown<std::logic_error>("Mismatched edge/node initialization", [](){ return graph_type{{{edge_init_type{1}}, {edge_init_type{0}}}, nodes{1.0}}; });
    }

    [[nodiscard]]
    static transition_graph make_weighted_transition_graph(regular_test& t)
    {
      using base_ops = dynamic_undirected_graph_operations<EdgeWeight, NodeWeight, maths::null_meta_data, EdgeStorageConfig, NodeWeightStorage>;
      using namespace undirected_graph;

      auto trg{base_ops::make_transition_graph(t)};

      check_initialization_exceptions(t);

      // 'unsortable_weight_graph_description::us_nodew'
      trg.add_node(make_and_check(t, t.report(""), {{}}, {{1.0, -1.0}}));

      // 'unsortable_weight_graph_description::us_node_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}}}, {{0.0}}));

      // 'unsortable_weight_graph_description::us_node_0w_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 1.0, -1.0}}}, {{0.0}}));

      // 'unsortable_weight_graph_description::us_node_0_0w'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report(""), {{{0, 0.0, 0.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}, {0, 1.0, -1.0}}}, {{0.0}})};
          t.check(equality, "Canonical ordering of weighted edges", graph_type{{{{0, 0.0, 0.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}}}, {{0.0}}}, g);
          return g;
        }());
      
      // 'unsortable_weight_graph_description::us_node_0w_0'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}}}, {{0.0}})};
          t.check(equality, "Canonical ordering of weighted edges", graph_type{{{{0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}}}, {{0.0}}}, g);
          return g;
        }());

      // 'unsortable_weight_graph_description::us_node_0w_1_node_0'
      trg.add_node(make_and_check(t, t.report(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {1, 0.0, 0.0}}, {{0, 0.0, 0.0}}}, {{}, {}}));

      // 'unsortable_weight_graph_description::us_node_0_1w_node_0w'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report(""), {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {1, 0.0, 0.0}}, {{0, 0.0, 0.0}}}, {{}, {}})};
          t.check(equality, "Canonical ordering of weighted edges", graph_type{{{{0, 1.0, -1.0}, {1, 0.0, 0.0}, {0, 1.0, -1.0}}, {{0, 0.0, 0.0}}}, {{}, {}}}, g);
          return g;
        }());

      // 'unsortable_weight_graph_description::us_node_1_node_1w_0,'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report(""), {{{1, 0.0, 0.0}}, {{0, 0.0, 0.0}, {1, 1.0, -1.0}, {1, 1.0, -1.0}}}, {{}, {}})};
          t.check(equality, "Canonical ordering of weighted edges", graph_type{{{{1, 0.0, 0.0}}, {{1, 1.0, -1.0}, {0, 0.0, 0.0}, {1, 1.0, -1.0}}}, {{}, {}}}, g);
          return g;
        }());

      // 'unsortable_weight_graph_description::us_node_1_1w_node_0_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 0.0, 0.0}, {1, 1.0, -1.0}}, {{0, 0.0, 0.0}, {0, 1.0, -1.0}}}, {{}, {}}));

      // 'unsortable_weight_graph_description::us_node_1w_1_node_0_0w'
      trg.add_node(make_and_check(t, t.report(""), {{{1, 1.0, -1.0}, {1, 0.0, 0.0}}, {{0, 0.0, 0.0}, {0, 1.0, -1.0}}}, {{}, {}}));

      // 'unsortable_weight_graph_description::us_node_1_1_1w_1w_node_0w_0w_0_0,'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t, t.report(""), {{{1, 0.0, 0.0}, {1, 0.0, 0.0}, {1, 1.0, -1.0}, {1, 1.0, -1.0}}, {{0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}}}, {{}, {}})};
          t.check(equality,
                  t.report("Canonical ordering of weighted edges"),
                  graph_type{{{{1, 0.0, 0.0}, {1, 1.0, -1.0}, {1, 1.0, -1.0}, {1, 0.0, 0.0}}, 
                              {{0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}}}, {{}, {}}},
                  g);

          return g;
        }());

      // 'unsortable_weight_graph_description::us_node_0w_1_1_1w_1w_node_0w_0w_0_0,'
      trg.add_node(
        [&t](){
          auto g{make_and_check(t,
                                t.report(""),
                                {{{0, 1.0, -1.0}, {0, 1.0, -1.0}, {1, 0.0, 0.0}, {1, 0.0, 0.0}, {1, 1.0, -1.0}, {1, 1.0, -1.0}},
                                 {{0, 1.0, -1.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 0.0, 0.0}}},
                                {{}, {}})};
          t.check(equality,
            t.report("Canonical ordering of weighted edges"),
            graph_type{{{{1, 0.0, 0.0}, {0, 1.0, -1.0}, {1, 1.0, -1.0}, {1, 1.0, -1.0}, {1, 0.0, 0.0}, {0, 1.0, -1.0}},
                        {{0, 1.0, -1.0}, {0, 0.0, 0.0}, {0, 1.0, -1.0}, {0, 0.0, 0.0}}}, {{}, {}}},
            g);

          return g;
        }());

      return trg;
    }
  };


}