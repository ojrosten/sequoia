////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "DynamicEmbeddedDirectedGraphUnweightedTest.hpp"

#include "sequoia/TestFramework/StateTransitionUtilities.hpp"

namespace sequoia::testing
{
  namespace
  {
    /// Convention: the indices following 'node' - separated by underscores - give the target node of the associated edges
    enum graph_description : std::size_t {
      empty = 0,

      // x
      node,

      //  />\
      //  \ /
      //   x
      node_0,

      //  />\ />\
      //  \ / \ /
      //     x
      node_0_0,

      //     />\/>\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0_0_interleaved,

      //  /<\
      //  \ /
      //   x
      node_0inv,

      //  /<\ /<\
      //  \ / \ /
      //     x
      node_0inv_0inv,

      //     /<\/<\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0inv_0inv_interleaved,

      //     />\/<\
      //    /  /\  \
      //    \ /  \ /
      //        x
      node_0_0inv_interleaved,

      //     />\/<\
      //    /  /\  \
      //    \ /  \ /
      //       x
      //      / \
      //      \>/
      //
      node_0inv_0_0inv_interleaved,

      //  x    x
      node_node,

      //  x ---> x
      node_1_node,

      //  x <--- x
      node_node_0,

      //   />\
      //   \ /
      //    x ---> x
      node_0_1_node,

      //  />\
      //  \ /
      //   x      x
      node_0_node,

      //      />\
      //      \ /
      //  x    x
      node_node_1,

      //      />\/<\
      //     /  /\  \
      //     \ /  \ /
      //  x      x
      node_node_1_1inv_interleaved,

      //      />\/<\
      //     /  /\  \
      //     \ /  \ /
      //  x     x
      //       / \
      //       \>/
      //
      node_node_1inv_1_1inv_interleaved,

      //      />\/<\
      //     /  /\  \
      //     \ /  \ /
      // x ---> x
      //       / \
      //       \>/
      //
      node_1_node_1inv_1_1inv_interleaved,

      //         />\/<\
      //        /  /\  \
      //        \ /  \ /
      // -> x ---> x-----
      //          / \
      //          \>/
      //
      node_1_node_1inv_1_0_1inv_interleaved,

      //  x ===> x
      node_1_1_node,

      // x ---> x
      //   <---
      node_1_node_0,

      // x ---> x
      //   <---
      node_1pos1_node_0pos0,

      //  x    x    x
      node_node_node,

      //  x ---> x    x
      node_1_node_node,

      //  x    x <--- x
      node_node_node_1,

      //  x ---> x ---> x
      node_1_node_2_node,

      //  x ---> x <--- x
      node_1_node_node_1,

      // -> x ---> x ---> x --
      node_1_node_2_node_0,

      //      />\
      //      \ /
      //  x    x    x
      node_node_1_node,

      //        />\
      //        \ /
      //  x ---> x    x
      node_1_node_1_node,


      //        />\
      //        \ /
      //  x ---> x    x
      //    <---
      node_1_node_1_0_node,

      //        />\
      //        \ /
      //  x ---> x ---> x
      //    <---
      node_1_node_1_0_2_node,

      //        />\
      //        \ /
      //  x ---> x ---> x
      node_1_node_1_2_node,

      //             />\
      //             \ /
      // -- x    x    x <-
      node_2_node_node_2,

      //     [2]
      //      x
      //    ^^  ^
      //   //    \
      //  //      \
      //  x  ===>  x
      // [0]      [1]
      node_1_1_2_2_node_2_node,

      //      [2]
      //       x
      //    ^^  \^
      //   //    \\
      //  //     `'\
      //  x  ===>  x
      // [0]      [1]
      node_1_1_2_2_node_2_node_1,

      //      [2]
      //       x
      //    ^^  \^
      //   //    \\
      //  //     `'\
      //  x  ===>  x
      // [0]      [1]
      node_2_2_1_1_node_2pos3_node_1,

      //  x [3]
      //  ^
      //  |
      //  |
      //  x ---> x ---> x
      // [0]    [1]    [2]
      node_3_1_node_2_node_node,

      // x ---> x    x <--- x
      node_1_node_node_node_2,

      // x --   x <-   -> x  -- x
      node_2_node_node_node_1
    };
  }

  [[nodiscard]]
  std::filesystem::path dynamic_embedded_directed_graph_unweighted_test::source_file() const
  {
    return std::source_location::current().file_name();
  }

  void dynamic_embedded_directed_graph_unweighted_test::run_tests()
  {
    using namespace maths;
    graph_test_helper<null_weight, null_weight, dynamic_embedded_directed_graph_unweighted_test> helper{*this};

    helper.run_tests<graph_flavour::directed_embedded>();
  }

  template
  <
    maths::graph_flavour GraphFlavour,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
  void dynamic_embedded_directed_graph_unweighted_test::execute_operations()
  {
    using ops = dynamic_embedded_directed_graph_operations<EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using graph_t = ops::graph_t;

    auto trg{ops::make_transition_graph(*this)};

    auto checker{
        [this](std::string_view description, const graph_t& obtained, const graph_t& prediction, const graph_t& parent, std::size_t host, std::size_t target) {
          check(equality, description, obtained, prediction);
          if(host != target) this->check_semantics(description, prediction, parent);
        }
    };

    transition_checker<graph_t>::check(report_line(""), trg, checker);
  }
}