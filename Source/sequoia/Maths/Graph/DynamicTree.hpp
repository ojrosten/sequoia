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

    tree() = default;

    // TO DO: think about depth-like vs breadth-like initialization
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

    // TO DO: think about whether this should/should not preserve initialization structure
    template<class... Args>
    size_type add_node(size_type parent, Args&&... args)
    {
      return base_type::tree_join(tree_link_direction_constant<TreeLinkDir>{}, parent, std::forward<Args>(args)...);
    }

    // insert_node

    // prune (subtle for backward edges)
  };

  template<class Tree>
  class basic_tree_adaptor
  {
  public:
    using value_type = Tree;

    using size_type = typename Tree::size_type;

    basic_tree_adaptor() = default;

    basic_tree_adaptor(Tree& tree, size_type node)
      : m_pTree{&tree}
      , m_Node{node}
    {};

    [[nodiscard]]
    Tree& tree()
      requires (!std::is_const_v<Tree>)
    {
      return *m_pTree;
    }

    [[nodiscard]]
    const Tree& tree() const
    {
      return *m_pTree;
    }

    [[nodiscard]]
    size_type node() const noexcept
    {
      return m_Node;
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
      return m_pTree == nullptr;
    }

    [[nodiscard]]
    explicit operator bool() const noexcept
    {
      return !empty() && (m_Node < m_pTree->order());
    }

    [[nodiscard]]
    friend bool operator==(const basic_tree_adaptor& lhs, const basic_tree_adaptor& rhs) noexcept = default;

    [[nodiscard]]
    friend bool operator!=(const basic_tree_adaptor& lhs, const basic_tree_adaptor& rhs) noexcept = default;
  private:
    Tree* m_pTree{};
    size_type m_Node{Tree::npos};
  };

  template<class Tree>
  using tree_adaptor = basic_tree_adaptor<Tree>;

  template<class Tree>
  using const_tree_adaptor = basic_tree_adaptor<const Tree>;

  template<std::input_or_output_iterator Iterator, class TreeAdaptor>
  class forest_from_tree_dereference_policy
  {
  public:
    using value_type          = typename std::iterator_traits<Iterator>::value_type;
    using proxy               = TreeAdaptor;
    using pointer             = typename std::iterator_traits<Iterator>::pointer;
    using reference           = typename std::iterator_traits<Iterator>::reference;
    using tree_reference_type = typename TreeAdaptor::value_type&;

    // TO DO: Hopefully get rid of this once MSVC bug fixed
    constexpr forest_from_tree_dereference_policy() = default;
    constexpr forest_from_tree_dereference_policy(tree_reference_type tree) : m_pTree{&tree} {}
    constexpr forest_from_tree_dereference_policy(const forest_from_tree_dereference_policy&) = default;

    [[nodiscard]]
    friend constexpr bool operator==(const forest_from_tree_dereference_policy&, const forest_from_tree_dereference_policy&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const forest_from_tree_dereference_policy&, const forest_from_tree_dereference_policy&) noexcept = default;
  protected:
    constexpr forest_from_tree_dereference_policy(forest_from_tree_dereference_policy&&) = default;

    ~forest_from_tree_dereference_policy() = default;

    constexpr forest_from_tree_dereference_policy& operator=(const forest_from_tree_dereference_policy&) = default;
    constexpr forest_from_tree_dereference_policy& operator=(forest_from_tree_dereference_policy&&) = default;

    [[nodiscard]]
    constexpr proxy get(reference ref) const
    {
      return {*m_pTree, ref.target_node()};
    }

    [[nodiscard]]
    static constexpr pointer get_ptr(reference ref) noexcept { return &ref; }
  private:
    TreeAdaptor m_Proxy;

    using tree_pointer_type = std::remove_reference_t<tree_reference_type>*;

    tree_pointer_type m_pTree{};
  };

  template<std::input_or_output_iterator Iterator, class Adaptor>
  using forest_from_tree_iterator = utilities::iterator<Iterator, forest_from_tree_dereference_policy<Iterator, Adaptor>>;
}
