////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/TestFramework/RegularTestCore.hpp"
#include "sequoia/Maths/Graph/DynamicTree.hpp"

namespace sequoia::testing
{
  template
  <
    maths::directed_flavour Directedness,
    maths::tree_link_direction TreeLinkDir,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
    requires (ownership::creator<EdgeWeightCreator> && ownership::creator<NodeWeightCreator>)
  struct detailed_equality_checker<sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>>
  {
    using tree_type = sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const tree_type& actual, const tree_type& prediction)
    {
      // e.g. check_equality("Description", logger, actual.method(), prediction.method());
    }
  };

  template
  <
    maths::directed_flavour Directedness,
    maths::tree_link_direction TreeLinkDir,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator,
    class NodeWeightCreator,
    class EdgeStorageTraits,
    class NodeWeightStorageTraits
  >
    requires (ownership::creator<EdgeWeightCreator> && ownership::creator<NodeWeightCreator>)
  struct equivalence_checker<sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>>
  {
    using tree_type = sequoia::maths::tree<Directedness, TreeLinkDir, EdgeWeight, NodeWeight, EdgeWeightCreator, NodeWeightCreator, EdgeStorageTraits, NodeWeightStorageTraits>;
    using size_type = typename tree_type::size_type;

    template<test_mode Mode>
    static void check(test_logger<Mode>& logger, const tree_type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      check_node(logger, 0, actual, prediction);
    }
  private:
    template<test_mode Mode>
    static size_type check_node(test_logger<Mode>& logger, size_type node, const tree_type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      if(node == tree_type::npos) return node;

      if(testing::check("Insufficient nodes detected while checking node " + std::to_string(node), logger, node < actual.order()))
      {
        check_equality("Node weight for node " + std::to_string(node), logger, actual.cbegin_node_weights()[node], prediction.node);

        if(check_num_edges(logger, node, actual, prediction))
        {
          auto edgeIter{actual.cbegin_edges(node)};
          if constexpr(TreeLinkDir == maths::tree_link_direction::symmetric)
          {
            if(node && (edgeIter != actual.cend_edges(node)))
              ++edgeIter;
          }

          for(const auto& child : prediction.children)
          {
            if(!check_edge(logger, edgeIter++, ++node, actual))
              return tree_type::npos;

            node = check_node(logger, node, actual, child);

            if(node == tree_type::npos) break;
          }

          return node;
        }
      }

      return tree_type::npos;
    }

    template<test_mode Mode>
    static bool check_num_edges(test_logger<Mode>& logger, size_type node, const tree_type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      using std::distance;

      if constexpr(TreeLinkDir == maths::tree_link_direction::forward)
      {
        return check_equality("Number of children for node " + std::to_string(node), logger, fixed_width_unsigned_cast(distance(actual.cbegin_edges(node), actual.cend_edges(node))), prediction.children.size());
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::backward)
      {
        static_assert(dependent_false<tree_type>::value, "Not yet implemented");
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::symmetric)
      {
        const auto num{
          [&actual,&logger,node](){
            const auto dist{fixed_width_unsigned_cast(distance(actual.cbegin_edges(node), actual.cend_edges(node)))};
            return (node && testing::check("Return edge detected", logger, dist > 0)) ? dist - 1 : dist;
          }()
        };

        return check_equality("Number of children for node " + std::to_string(node), logger, num, prediction.children.size());
      }
      else
      {
        static_assert(dependent_false<tree_type>::value);
      }
    }

    template<test_mode Mode, std::input_or_output_iterator EdgeIter>
    static bool check_edge(test_logger<Mode>& logger, EdgeIter iter, size_type nodeCounter, const tree_type& actual)
    {
      using std::distance;

      if constexpr(TreeLinkDir == maths::tree_link_direction::forward)
      {
        const auto parent{iter.partition_index()};
        const auto dist{distance(actual.cbegin_edges(parent), iter)};
        const auto mess{std::string{"Index for child "}.append(std::to_string(dist)).append(" of node ").append(std::to_string(parent))};

        return check_equality(mess, logger, iter->target_node(), nodeCounter);
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::backward)
      {
        static_assert(dependent_false<tree_type>::value, "Not yet implemented");
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::symmetric)
      {
        const auto parent{iter.partition_index()};
        const auto dist{distance(actual.cbegin_edges(parent), iter)};
        const auto mess{std::string{"Index for child "}.append(std::to_string(dist)).append(" of node ").append(std::to_string(parent))};

        return check_equality(mess, logger, iter->target_node(), nodeCounter);
      }
      else
      {
        static_assert(dependent_false<tree_type>::value);
      }
    }
  };

  // TO DO: non-null edge weights
}
