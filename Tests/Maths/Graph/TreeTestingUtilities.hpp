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
      check_node(logger, 0, tree_type::npos, actual, prediction);
    }
  private:
    using edge_iterator = typename tree_type::const_edge_iterator;

    template<test_mode Mode>
    static size_type check_node(test_logger<Mode>& logger, size_type node, size_type parent, const tree_type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      if(node == tree_type::npos) return node;

      if(testing::check("Insufficient nodes detected while checking node " + std::to_string(node), logger, node < actual.order()))
      {
        check_equality("Node weight for node " + std::to_string(node), logger, actual.cbegin_node_weights()[node], prediction.node);

        if(auto optIter{check_num_edges(logger, node, parent, actual, prediction)})
        {
          for(const auto& child : prediction.children)
          {
            if(!check_edge(logger, (*optIter)++, ++node, actual))
              return tree_type::npos;

            node = check_node(logger, node, (*optIter).partition_index(), actual, child);

            if(node == tree_type::npos) break;
          }

          return node;
        }
      }

      return tree_type::npos;
    }

    template<test_mode Mode>
    static std::optional<edge_iterator> check_num_edges(test_logger<Mode>& logger, size_type node, [[maybe_unused]] size_type parent, const tree_type& actual, const maths::tree_initializer<NodeWeight>& prediction)
    {
      using std::distance;

      if constexpr(TreeLinkDir == maths::tree_link_direction::forward)
      {
        if(check_equality("Number of children for node " + std::to_string(node), logger, fixed_width_unsigned_cast(distance(actual.cbegin_edges(node), actual.cend_edges(node))), prediction.children.size()))
          return actual.cbegin_edges(node);
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::backward)
      {
        static_assert(dependent_false<tree_type>::value, "Not yet implemented");
      }
      else if constexpr(TreeLinkDir == maths::tree_link_direction::symmetric)
      {
        const auto begin{actual.cbegin_edges(node)};
        const auto dist{fixed_width_unsigned_cast(distance(begin, actual.cend_edges(node)))};
        using dist_t = decltype(dist);

        const auto num{
          [&logger,parent,dist,begin]() -> std::optional<dist_t> {
            if(parent == tree_type::npos) return dist;

            if(   testing::check("Return edge detected", logger, dist > 0)
               && check_equality("Return edge target", logger, begin->target_node(), parent))
              return dist - 1;

            return std::nullopt;
          }()
        };

        if(    num.has_value()
            && check_equality("Number of children for node " + std::to_string(node), logger, num.value(), prediction.children.size()))
        {
          return num < dist ? std::next(begin) : begin;
        }
      }
      else
      {
        static_assert(dependent_false<tree_type>::value);
      }

      return std::nullopt;
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
