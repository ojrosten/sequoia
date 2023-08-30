////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

#include "../DynamicGraphTestingUtilities.hpp"

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace undirected_graph{
    // Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    // Letters near the start of the alphabet distinguish meta-data values
    enum meta_data_graph_description : std::size_t {

      empty = 0,

      // x
      node,

      //  /\
      //  \/
      //  x
      node_0a_0a,

      //  /\
      //  \/
      //  x
      node_0a_0b,

      //  /\
      //  \/
      //  x
      node_0b_0a,
    };
  }

  template
  <
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData,
    class EdgeStorageConfig,
    class NodeWeightStorage
  >
  class dynamic_undirected_graph_meta_data_operations
  {
   public:
    using graph_t            = maths::undirected_graph<EdgeWeight, NodeWeight, EdgeMetaData, EdgeStorageConfig, NodeWeightStorage>;
    using edge_t             = typename graph_t::edge_init_type;
    using node_weight_type   = typename graph_t::node_weight_type;
    using edges_equivalent_t = std::initializer_list<std::initializer_list<edge_t>>;
    using transition_graph   = typename transition_checker<graph_t>::transition_graph;

    constexpr static bool has_shared_weight{EdgeStorageConfig::edge_sharing == maths::edge_sharing_preference::shared_weight};

    static void execute_operations(regular_test& t)
    {
      auto trg{make_meta_data_transition_graph(t)};

      auto checker{
          [&t](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
            t.check(equality, description, obtained, prediction);
            if(host != target) t.check_semantics(description, prediction, parent);
          }
      };

      transition_checker<graph_t>::check(report_line(""), trg, checker);
    }

    [[nodiscard]]
    static graph_t make_and_check(regular_test& t, std::string_view description, edges_equivalent_t init)
    {
      return graph_initialization_checker<graph_t>::make_and_check(t, description, init);
    }

    static void check_initialization_exceptions(regular_test& t)
    {
      using namespace maths;

      // One node
      t.check_exception_thrown<std::out_of_range>(t.report_line("Target index of edge out of range"), [](){ return graph_t{{edge_t{1, 0.5f}}}; });
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched loop"), [](){ return graph_t{{edge_t{0, 0.5f}}}; });

      // Two nodes
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched partial edges"), [](){ return graph_t{{edge_t{1, 0.5f}}, {edge_t{1, -0.5f}}}; });
      t.check_exception_thrown<std::logic_error>(t.report_line("Mismatched loop"), [](){ return graph_t{{edge_t{1, 0.5f}}, {edge_t{0, 0.6f}, edge_t{1, -0.5f}}}; });
    }

    [[nodiscard]]
    static transition_graph make_meta_data_transition_graph(regular_test& t)
    {
      using meta_data_t = EdgeMetaData;
      using namespace undirected_graph;

      check_initialization_exceptions(t);

      return transition_graph{
        {
          {  // begin 'meta_data_graph_description::empty'
          }, // end 'meta_data_graph_description::empty' 
          {  // begin 'meta_data_graph_description::node'
            {
              meta_data_graph_description::node_0a_0b,
              t.report_line("Add loop"),
              [](graph_t g) -> graph_t {
                g.join(0, 0, 0.0f, 0.5f);
                return g;
              }
            },
            {
              meta_data_graph_description::node_0b_0a,
              t.report_line("Add loop"),
              [](graph_t g) -> graph_t {
                g.join(0, 0, 0.5f, 0.0f);
                return g;
              }
            }
          }, // end 'meta_data_graph_description::node'
          {  // begin 'meta_data_graph_description::node_0a_0a'
            {
              meta_data_graph_description::node_0b_0a,
              t.report_line("Set edge meta data"),
              [](graph_t g) -> graph_t {
                g.set_edge_meta_data(g.cbegin_edges(0), 0.5f);
                return g;
              }
            },
            {
              meta_data_graph_description::node_0b_0a,
              t.report_line("Set edge meta data"),
              [](graph_t g) -> graph_t {
                g.set_edge_meta_data(g.cbegin_edges(0), meta_data_t{0.5f});
                return g;
              }
            },
            {
              meta_data_graph_description::node_0b_0a,
              t.report_line("Mutate edge meta data"),
              [&t](graph_t g) -> graph_t {
                t.check(equality, report_line("Mutate return value"), g.mutate_edge_meta_data(g.cbegin_edges(0), [](meta_data_t& m) { m += 0.5f; return 42; }), 42);
                return g;
              }
            },
            {
              meta_data_graph_description::node_0a_0b,
              t.report_line("Set meta data via reverse iterator"),
              [](graph_t g) -> graph_t {
                g.set_edge_meta_data(g.crbegin_edges(0), 0.5f);
                return g;
              }
            },
            {
              meta_data_graph_description::node_0a_0b,
              t.report_line("Mutate edge meta data via reverse iterator"),
              [&t](graph_t g) -> graph_t {
                t.check(equality, report_line("Mutate return value"), g.mutate_edge_meta_data(g.crbegin_edges(0), [](meta_data_t& m) { m += 0.5f; return 42; }), 42);
                return g;
              }
            }
          }, // end 'meta_data_graph_description::node_0a_0a'
          {  // begin 'meta_data_graph_description::node_0a_0b'
          }, // end 'meta_data_graph_description::node_0a_0b'
          {  // begin 'meta_data_graph_description::node_0b_0a'
          }, // end 'meta_data_graph_description::node_0b_0a'
        },
        {
          //  'empty'
          make_and_check(t, t.report_line(""), {}),

          //  'node'
          make_and_check(t, t.report_line(""), {{}}),

          // 'node_0a_0a
          make_and_check(t, t.report_line(""), {{{0, 0.0f}, {0, 0.0f}}}),

          // 'node_0a_0b
          make_and_check(t, t.report_line(""), {{{0, 0.0f}, {0, 0.5f}}}),

          // 'node_0b_0a
          make_and_check(t, t.report_line(""), {{{0, 0.5f}, {0, 0.0f}}}),
        }
      };
    }
  };
}