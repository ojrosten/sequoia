////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Restriction of Dynamic Graphs to Trees 
*/

#include "sequoia/Maths/Graph/DynamicGraph.hpp"

namespace sequoia::maths
{
  template
  <
    directed_flavour Directedness,
    tree_link_direction TreeLinkDir,
    class EdgeWeight,
    class NodeWeight,
    class EdgeWeightCreator = ownership::spawner<EdgeWeight>,
    class NodeWeightCreator = ownership::spawner<NodeWeight>,
    class EdgeStorageTraits = bucketed_edge_storage_traits,
    class NodeWeightStorageTraits = node_weight_storage_traits<NodeWeight>
  >
    requires (      ownership::creator<EdgeWeightCreator> && ownership::creator<NodeWeightCreator>
               && ((TreeLinkDir == tree_link_direction::symmetric) || (Directedness == directed_flavour::directed)))
    class tree final : public
      graph_base
      <
        graph_impl::to_graph_flavour<Directedness>(),
        EdgeWeight,
        NodeWeight,
        EdgeWeightCreator,
        NodeWeightCreator,
        EdgeStorageTraits,
        NodeWeightStorageTraits
      >
  {
  public:
    using base_type =
      graph_base
      <
        graph_impl::to_graph_flavour<Directedness>(),
        EdgeWeight,
        NodeWeight,
        EdgeWeightCreator,
        NodeWeightCreator,
        EdgeStorageTraits,
        NodeWeightStorageTraits
      >;

    using size_type = typename base_type::size_type;

    constexpr static tree_link_direction link_dir{TreeLinkDir};

    tree(const tree&)     = default;
    tree(tree&&) noexcept = default;

    tree& operator=(const tree&)     = default;
    tree& operator=(tree&&) noexcept = default;

    tree(tree_initializer<NodeWeight> tree)
      : base_type{tree, tree_link_direction_constant<TreeLinkDir>{}}
    {}

    void swap(tree& rhs) noexcept(noexcept(base_type::swap(rhs)))
    {
      base_type::swap(rhs);
    }

    friend void swap(tree& lhs, tree& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    template<class... Args>
    size_type add_node(size_type parent, Args&&... args)
    {
      if(const auto node{base_type::add_node(std::forward<Args>(args)...)})
      {
        if constexpr(link_dir != tree_link_direction::backward)
        {
          join(parent, node);
        }
        
        if constexpr((link_dir != tree_link_direction::forward) && (Directedness != directed_flavour::undirected))
        {
          join(node, parent);
        }
      }
    }

    // prune (subtle for backward edges)
  };
}