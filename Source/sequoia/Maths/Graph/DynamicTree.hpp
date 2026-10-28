////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2021.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file
    \brief Restriction of Dynamic Graphs to Trees 
*/

#include "sequoia/Core/Numeric/IntegerConversions.hpp"
#include "sequoia/Maths/Graph/DynamicGraph.hpp"
#include "sequoia/Maths/Graph/DynamicGraphTraversals.hpp"
#include "sequoia/Maths/Graph/GraphErrors.hpp"

#include <cstddef>
#include <ranges>

namespace sequoia::maths
{
  /** \brief The base of `directed_tree` and `undirected_tree`.

      A tree's root is node 0. A node operation which would move the root
      throws std::out_of_range instead.
   */
  template
  <
    graph_flavour Flavour,
    tree_link_direction TreeLinkDir,
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData,
    class EdgeStorageConfig = bucketed_edge_storage_config,
    class NodeWeightStorage = node_storage<NodeWeight>
  >
    requires ((TreeLinkDir == tree_link_direction::symmetric) || (Flavour == graph_flavour::directed))
    class tree_base : public
      graph_base
      <
        Flavour,
        EdgeWeight,
        NodeWeight,
        EdgeMetaData,
        EdgeStorageConfig,
        NodeWeightStorage
      >
  {
      using base_type =
        graph_base
        <
          Flavour,
          EdgeWeight,
          NodeWeight,
          EdgeMetaData,
          EdgeStorageConfig,
          NodeWeightStorage
        >;

      template<class Compare>
      constexpr static bool nodes_sortable_by_v{base_type::template nodes_sortable_by_v<Compare>};
  public:
    using size_type       = base_type::size_type;
    using edge_index_type = base_type::edge_index_type;

    constexpr static tree_link_direction link_dir{TreeLinkDir};

    tree_base() = default;

    tree_base(const tree_base&)            = default;
    tree_base& operator=(const tree_base&) = default;

    // TO DO: think about depth-like vs breadth-like initialization
    tree_base(tree_initializer<NodeWeight> tree)
      : base_type{tree, tree_link_direction_constant<TreeLinkDir>{}}
    {}

    template<class... Args>
      requires is_initializable_v<NodeWeight, Args...>
    size_type add_node(const size_type parent, Args&&... args)
    {
      return base_type::add_node_to_tree(tree_link_direction_constant<TreeLinkDir>{}, parent, std::forward<Args>(args)...);
    }

    /** \throws std::out_of_range, leaving the tree unchanged, if `pos` is 0
        and the tree is not empty, since the root is node 0.
     */
    template<class... Args>
      requires is_initializable_v<NodeWeight, Args...>
    size_type insert_node(const size_type pos, const size_type parent, Args&&... args)
    {
      if((pos == 0) && (this->order() > 0))
        throw std::out_of_range{graph_errors::root_displacement_message("insert_node")};

      return base_type::insert_node_to_tree(tree_link_direction_constant<TreeLinkDir>{}, pos, parent, std::forward<Args>(args)...);
    }

    /** \brief Removes `node` and its descendants.

        \throws std::out_of_range if the tree lacks `node`.
     */
    void prune(const size_type node)
    {
      graph_errors::check_node_index_range("prune", this->order(), node);

      const auto isInSubtree{subtree_membership(node, tree_link_direction_constant<link_dir>{})};

      // Highest index first: erasing a node shifts every higher index down
      for(const auto i : std::views::iota(size_type{}, this->order()) | std::views::reverse)
      {
        if(isInSubtree[i])
          this->erase_node(i);
      }
    }

    /** \throws std::out_of_range, leaving the tree unchanged, if `i` and `j`
        differ and either is 0, since the root is node 0.
     */
    constexpr void swap_nodes(const edge_index_type i, const edge_index_type j)
    {
      if((i != j) && ((i == 0) || (j == 0)))
        throw std::out_of_range{graph_errors::root_displacement_message("swap_nodes")};

      base_type::swap_nodes(i, j);
    }

    /** \brief Sorts all the nodes, as `sort_nodes(0, order(), c)` does.

        A symmetric tree does not have this overload.
     */
    template<class Compare>
      requires (link_dir != tree_link_direction::symmetric) && nodes_sortable_by_v<Compare>
    constexpr void sort_nodes(Compare c)
    {
      sort_nodes(edge_index_type{}, numeric::unchecked_conversion_to<edge_index_type>(this->order()), std::move(c));
    }

    /** \brief Sorts the nodes `first` to `last - 1`, as
               `graph_primitive::sort_nodes` does.

        \throws std::out_of_range, leaving the tree unchanged, if node 0, the
        root, is in the range and `c` orders some node of the range before it.
     */
    template<class Compare>
      requires nodes_sortable_by_v<Compare>
    constexpr void sort_nodes(const edge_index_type first, const edge_index_type last, Compare c)
    {
      if(last <= first)
        return;

      auto destinations{base_type::node_sort_destinations(first, last, std::move(c))};
      if(destinations[0] != 0)
        throw std::out_of_range{graph_errors::root_displacement_message("sort_nodes")};

      base_type::permute_nodes(std::move(destinations));
    }

    using base_type::swap_edges;
    using base_type::sort_edges;
    using base_type::stable_sort_edges;
  protected:
    ~tree_base() = default;

    tree_base(tree_base&&) noexcept = default;
    tree_base& operator=(tree_base&&) noexcept = default;
  private:
    /** \brief Returns, for each node, whether it is `node` or a descendant of
               it, in the tree rooted at `searchStart`.
     */
    [[nodiscard]]
    std::vector<bool> subtree_membership_by_search(const size_type node, const size_type searchStart) const
    {
      std::vector<bool> isInSubtree(this->order(), false);
      bool withinSubtree{};

      traverse(depth_first,
               *this,
               ignore_disconnected_t{searchStart},
               [node, &isInSubtree, &withinSubtree](const auto n) {
                 if(n == node)
                   withinSubtree = true;

                 isInSubtree[n] = withinSubtree;
               },
               [node, &withinSubtree](const auto n) {
                 if(n == node)
                   withinSubtree = false;
               });

      return isInSubtree;
    }

    [[nodiscard]]
    std::vector<bool> subtree_membership(const size_type node, forward_tree_type) const
    {
      return subtree_membership_by_search(node, node);
    }

    [[nodiscard]]
    std::vector<bool> subtree_membership(const size_type node, symmetric_tree_type) const
    {
      return subtree_membership_by_search(node, size_type{});
    }

    [[nodiscard]]
    std::vector<bool> subtree_membership(const size_type node, backward_tree_type) const
    {
      std::vector<bool> isInSubtree(this->order(), false);
      isInSubtree[node] = true;

      for(size_type i{}; i < this->order(); ++i)
      {
        if(!isInSubtree[i])
          mark_if_descendant(i, isInSubtree);
      }

      return isInSubtree;
    }

    void mark_if_descendant(const size_type node, std::vector<bool>& isInSubtree) const
    {
      if(auto begin{this->cbegin_edges(node)};  begin != this->cend_edges(node))
      {
        const auto target{begin->target_node()};
        if(isInSubtree[target])
        {
          isInSubtree[node] = true;
        }
        else
        {
          mark_if_descendant(target, isInSubtree);
          if(isInSubtree[target])
            isInSubtree[node] = true;
        }
      }
    }
  };

  template
  <
    tree_link_direction TreeLinkDir,
    class EdgeWeight,
    class NodeWeight,
    class EdgeStorageConfig = bucketed_edge_storage_config,
    class NodeWeightStorage = node_storage<NodeWeight>
  >
  class directed_tree final : public
    tree_base
    <
      graph_flavour::directed,
      TreeLinkDir,
      EdgeWeight,
      NodeWeight,
      null_meta_data,
      EdgeStorageConfig,
      NodeWeightStorage
    >
  {
  public:
    using tree_base<
            graph_flavour::directed,
            TreeLinkDir,
            EdgeWeight,
            NodeWeight,
            null_meta_data,
            EdgeStorageConfig,
            NodeWeightStorage
        >::tree_base;
  };

  template
  <
    tree_link_direction TreeLinkDir,
    class EdgeWeight,
    class NodeWeight,
    class EdgeMetaData      = null_meta_data,
    class EdgeStorageConfig = bucketed_edge_storage_config,
    class NodeWeightStorage = node_storage<NodeWeight>
  >
    requires (TreeLinkDir == tree_link_direction::symmetric)
  class undirected_tree final : public
    tree_base
    <
      graph_flavour::undirected,
      TreeLinkDir,
      EdgeWeight,
      NodeWeight,
      EdgeMetaData,
      EdgeStorageConfig,
      NodeWeightStorage
    >
  {
  public:
    using tree_base<
            graph_flavour::undirected,
            TreeLinkDir,
            EdgeWeight,
            NodeWeight,
            EdgeMetaData,
            EdgeStorageConfig,
            NodeWeightStorage
        >::tree_base;
  };

  template<class Tree>
  class basic_tree_adaptor
  {
  public:
    using value_type = Tree;

    using size_type = Tree::size_type;

    basic_tree_adaptor() = default;

    basic_tree_adaptor(Tree& tree, size_type node)
      : m_pTree{&tree}
      , m_Node{node}
    {};

    [[nodiscard]]
    Tree& tree()
      requires (!std::is_const_v<Tree>)
    {
      if(!m_pTree) throw std::logic_error{"Tree not found"};

      return *m_pTree;
    }

    [[nodiscard]]
    const Tree& tree() const
    {
      if(!m_pTree) throw std::logic_error{"Tree not found"};

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
  private:
    Tree* m_pTree{};
    size_type m_Node{Tree::npos};
  };

  template<class Tree>
  using tree_adaptor = basic_tree_adaptor<Tree>;

  template<class Tree>
  using const_tree_adaptor = basic_tree_adaptor<const Tree>;

  /** \throws std::out_of_range if `adaptor` is at a node its tree lacks. */
  template<dynamic_tree T>
  [[nodiscard]]
  auto root_weight_iter(const basic_tree_adaptor<T>& adaptor)
  {
    const auto& tree{adaptor.tree()};
    graph_errors::check_node_index_range("root_weight_iter", tree.order(), adaptor.node());

    return tree.cbegin_node_weights() + numeric::unchecked_conversion_to<std::ptrdiff_t>(adaptor.node());
  }

  /** \throws std::out_of_range if `adaptor` is at a node its tree lacks. */
  template<dynamic_tree T>
  [[nodiscard]]
  const auto& root_weight(const basic_tree_adaptor<T>& adaptor)
  {
    return *root_weight_iter(adaptor);
  }

  /** \throws std::out_of_range if `adaptor` is at a node its tree lacks. */
  template<dynamic_tree T, class Fn>
    requires std::invocable<Fn&, typename T::node_weight_type&>
  std::invoke_result_t<Fn&, typename T::node_weight_type&> mutate_root_weight(basic_tree_adaptor<T>& adaptor, Fn fn)
  {
    return adaptor.tree().mutate_node_weight(root_weight_iter(adaptor), std::move(fn));
  }

  template<std::input_or_output_iterator Iterator, class TreeAdaptor>
  class forest_from_tree_dereference_policy
  {
  public:
    using proxy               = TreeAdaptor;
    using value_type          = proxy;
    using reference           = proxy;
    using tree_reference_type = TreeAdaptor::value_type&;

    constexpr forest_from_tree_dereference_policy() = default;
    constexpr forest_from_tree_dereference_policy(tree_reference_type tree) : m_pTree{&tree} {}
    constexpr forest_from_tree_dereference_policy(const forest_from_tree_dereference_policy&) = default;

    [[nodiscard]]
    constexpr proxy get(Iterator i) const
    {
      return {*m_pTree, i->target_node()};
    }

    [[nodiscard]]
    friend constexpr bool operator==(const forest_from_tree_dereference_policy&, const forest_from_tree_dereference_policy&) noexcept = default;
  protected:
    constexpr forest_from_tree_dereference_policy(forest_from_tree_dereference_policy&&) = default;

    ~forest_from_tree_dereference_policy() = default;

    constexpr forest_from_tree_dereference_policy& operator=(const forest_from_tree_dereference_policy&) = default;
    constexpr forest_from_tree_dereference_policy& operator=(forest_from_tree_dereference_policy&&) = default;
  private:
    using tree_pointer_type = std::remove_reference_t<tree_reference_type>*;

    tree_pointer_type m_pTree{};
  };

  template<std::input_or_output_iterator Iterator, class Adaptor>
  using forest_from_tree_iterator = utilities::iterator<Iterator, forest_from_tree_dereference_policy<Iterator, Adaptor>>;


  template<std::input_or_output_iterator Iterator, class TreeAdaptor>
  class forest_dereference_policy
  {
  public:
    using proxy      = TreeAdaptor;
    using value_type = proxy;
    using reference  = proxy;

    constexpr forest_dereference_policy() = default;
    constexpr forest_dereference_policy(const forest_dereference_policy&) = default;

    [[nodiscard]]
    constexpr static proxy get(Iterator i)
    {
      return {*i, 0};
    }

    [[nodiscard]]
    friend constexpr bool operator==(const forest_dereference_policy&, const forest_dereference_policy&) noexcept = default;
  protected:
    constexpr forest_dereference_policy(forest_dereference_policy&&) = default;

    ~forest_dereference_policy() = default;

    constexpr forest_dereference_policy& operator=(const forest_dereference_policy&) = default;
    constexpr forest_dereference_policy& operator=(forest_dereference_policy&&) = default;
  };

  template<std::input_or_output_iterator Iterator, class Adaptor>
  using forest_iterator = utilities::iterator<Iterator, forest_dereference_policy<Iterator, Adaptor>>;

  /** \brief Returns the subtrees rooted at `node`'s children, as adaptors of
             `tree`.

      \throws std::out_of_range if `tree` lacks `node`.
   */
  template<dynamic_tree T>
    requires (T::link_dir == tree_link_direction::forward)
  [[nodiscard]]
  auto forest_beneath(const T& tree, typename T::size_type node)
  {
    using iterator = forest_from_tree_iterator<typename T::const_edge_iterator, const_tree_adaptor<T>>;

    const auto edges{tree.cedges(node)};
    return std::ranges::subrange{iterator{edges.begin(), tree}, iterator{edges.end(), tree}};
  }

  /** \brief Returns adaptors of `trees`, one per tree, at node 0. */
  template<std::ranges::input_range Trees>
    requires dynamic_tree<std::ranges::range_value_t<Trees>>
  [[nodiscard]]
  auto forest_of(const Trees& trees)
  {
    using iterator = forest_iterator<std::ranges::iterator_t<const Trees>, const_tree_adaptor<std::ranges::range_value_t<Trees>>>;

    return std::ranges::subrange{iterator{std::ranges::begin(trees)}, iterator{std::ranges::end(trees)}};
  }
}
