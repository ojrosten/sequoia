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
    class EdgeWeightCreator       = object::faithful_producer<EdgeWeight>,
    class NodeWeightCreator       = object::faithful_producer<NodeWeight>,
    class EdgeStorageTraits       = bucketed_edge_storage_traits,
    class NodeWeightStorageTraits = node_weight_storage_traits<NodeWeight>
  >
    requires (      object::creator<EdgeWeightCreator> && object::creator<NodeWeightCreator>
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

    void swap(tree& rhs) noexcept(noexcept(this->base_type::swap(rhs)))
    {
      base_type::swap(rhs);
    }

    friend void swap(tree& lhs, tree& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
      lhs.swap(rhs);
    }

    template<class... Args>
      requires is_initializable_v<NodeWeight, Args...>
    size_type add_node(const size_type parent, Args&&... args)
    {
      return base_type::add_node_to_tree(tree_link_direction_constant<TreeLinkDir>{}, parent, std::forward<Args>(args)...);
    }

    template<class... Args>
      requires is_initializable_v<NodeWeight, Args...>
    size_type insert_node(const size_type pos, const size_type parent, Args&&... args)
    {
      return base_type::insert_node_to_tree(tree_link_direction_constant<TreeLinkDir>{}, pos, parent, std::forward<Args>(args)...);
    }

    void prune(const size_type node)
    {
      prune(node, tree_link_direction_constant<link_dir>{});
    }

    using base_type::swap_edges;
    using base_type::sort_edges;
    using base_type::swap_nodes;
    using base_type::sort_nodes;
  private:
    void prune(const size_type node, forward_tree_type ftt)
    {
      while(std::ranges::distance(this->crbegin_edges(node), this->crend_edges(node)))
      {
        const auto target{this->crbegin_edges(node)->target_node()};
        prune(target, ftt);
      }

      this->erase_node(node);
    }

    void prune(const size_type node, symmetric_tree_type stt)
    {
      const std::ptrdiff_t offset{node == 0 ? 0 : 1};

      while(std::ranges::distance(this->crbegin_edges(node), this->crend_edges(node)) > offset)
      {
        const auto target{this->crbegin_edges(node)->target_node()};
        prune(target, stt);
      }

      this->erase_node(node);
    }

    void prune(const size_type node, backward_tree_type btt)
    {
      if(node >= this->order()) return;

      std::vector<bool> toDelete(this->order(), false);
      toDelete[node] = true;

      for(size_type i{}; i < this->order(); ++i)
      {
        if(!toDelete[i]) prune(i, toDelete, btt);
      }

      for(size_type i{}; i < toDelete.size(); ++i)
      {
        // Erase nodes with the highest indicies first
        const auto j{toDelete.size() - 1 - i};
        if(toDelete[j]) this->erase_node(j);
      }
    }

    void prune(const size_type node, std::vector<bool>& toDelete, backward_tree_type btt)
    {
      if(auto begin{this->cbegin_edges(node)};  begin != this->cend_edges(node))
      {
        const auto target{begin->target_node()};
        if(toDelete[target])
        {
          toDelete[node] = true;
        }
        else
        {
          prune(target, toDelete, btt);
          if(toDelete[target]) toDelete[node] = true;
        }
      }
    }
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

  template<dynamic_tree T>
  [[nodiscard]]
  auto root_weight_iter(const basic_tree_adaptor<T>& adaptor)
  {
    return adaptor.tree().cbegin_node_weights() + adaptor.node();
  }

  template<dynamic_tree T>
  [[nodiscard]]
  const auto& root_weight(const basic_tree_adaptor<T>& adaptor)
  {
    return adaptor.tree().cbegin_node_weights()[adaptor.node()];
  }

  template<dynamic_tree T, std::invocable<typename T::node_weight_type&> Fn>
  void mutate_root_weight(basic_tree_adaptor<T>& adaptor, Fn fn)
  {
    adaptor.tree().mutate_node_weight(root_weight_iter(adaptor), fn);
  }

  template<std::input_or_output_iterator Iterator, class TreeAdaptor>
  class forest_from_tree_dereference_policy
  {
  public:
    using proxy               = TreeAdaptor;
    using value_type          = proxy;
    using reference           = proxy;
    using tree_reference_type = typename TreeAdaptor::value_type&;

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
}
