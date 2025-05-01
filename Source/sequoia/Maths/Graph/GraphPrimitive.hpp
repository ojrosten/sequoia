////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Underlying class for the various different graph flavour.
 */

#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Maths/Graph/Connectivity.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"
#include "sequoia/Core/ContainerUtilities/AssignmentUtilities.hpp"

namespace sequoia
{
  namespace maths
  {
    template<network Connectivity, class Nodes>
    class SEQUOIA_MSVC_EMPTY_BASE_HACK graph_primitive;

    namespace graph_impl
    {
      template<network Connectivity, class Nodes>
      class graph_dereference_policy
      {
      public:
        using graph_type      = graph_primitive<Connectivity, Nodes>;
        using index_type      = typename graph_type::edge_index_type;
        using difference_type = std::make_signed_t<index_type>;
        using value_type      = index_type;
        using reference       = index_type;

        [[nodiscard]]
        constexpr graph_type& graph() noexcept
        {
          return *m_Graph;
        }

        [[nodiscard]]
        constexpr const graph_type& graph() const noexcept
        {
          return *m_Graph;
        }

        [[nodiscard]]
        constexpr static reference get(index_type i) { return i; }
      protected:
        constexpr graph_dereference_policy() = default;

        constexpr graph_dereference_policy(graph_type& g) : m_Graph{&g} {}

        constexpr graph_dereference_policy(const graph_dereference_policy&)     = default;
        constexpr graph_dereference_policy(graph_dereference_policy&&) noexcept = default;
        
        ~graph_dereference_policy() = default;
        
        constexpr graph_dereference_policy& operator=(const graph_dereference_policy&)     = default;
        constexpr graph_dereference_policy& operator=(graph_dereference_policy&&) noexcept = default;

      private:
        graph_type* m_Graph{};
      };

      /// Designed to swap nodes, when used in conjunction with sequoia::swap,
      /// courtesy of the overload of iter_swap, below.
      /// This is a pseudo iterator since it doesn't have reference semantics
      template<network Connectivity, class Nodes>
      using pseudo_iterator = utilities::iterator<typename graph_primitive<Connectivity, Nodes>::edge_index_type, graph_dereference_policy<Connectivity, Nodes>>;
    }

    template<network Connectivity, class Nodes>
    constexpr void iter_swap(graph_impl::pseudo_iterator<Connectivity, Nodes> a, graph_impl::pseudo_iterator<Connectivity, Nodes> b)
    {
      a.graph().swap_nodes(a.base_iterator(), b.base_iterator());
    }

    template<class NodeWeight>
    struct tree_initializer
    {
      NodeWeight node;
      std::initializer_list<tree_initializer> children{};
    };

    template<class NodeWeight>
      requires std::is_empty_v<NodeWeight>
    struct tree_initializer<NodeWeight>
    {
      std::initializer_list<tree_initializer> children{};
    };

    enum class tree_link_direction {symmetric, forward, backward};

    template<tree_link_direction d>
    struct tree_link_direction_constant : std::integral_constant<tree_link_direction, d> {};

    using symmetric_tree_type = tree_link_direction_constant<tree_link_direction::symmetric>;
    using forward_tree_type   = tree_link_direction_constant<tree_link_direction::forward>;
    using backward_tree_type  = tree_link_direction_constant<tree_link_direction::backward>;

    template<network Connectivity, class Nodes>
    class SEQUOIA_MSVC_EMPTY_BASE_HACK graph_primitive : public Connectivity, public Nodes
    {
    private:
      using node_weight_type = typename Nodes::weight_type;

    public:
      using connectivity_type = Connectivity;
      using nodes_type        = Nodes;
      using edge_init_type    = typename Connectivity::edge_init_type;
      using edge_index_type   = typename Connectivity::edge_index_type;
      using size_type         = std::common_type_t<typename Connectivity::size_type, typename Nodes::size_type>;
      using pseudo_iterator   = graph_impl::pseudo_iterator<Connectivity, Nodes>;

      using edges_initializer = std::initializer_list<std::initializer_list<edge_init_type>>;

      constexpr graph_primitive() = default;

      constexpr graph_primitive(edges_initializer edges)
        requires std::is_empty_v<node_weight_type>
        : Connectivity{edges}
        , Nodes{}
      {}

      constexpr graph_primitive(edges_initializer edges)
        requires (!std::is_empty_v<node_weight_type> && !heterogeneous_nodes<graph_primitive> && std::is_default_constructible_v<node_weight_type>)
        : Connectivity{edges}
        , Nodes(edges.size())
      {}

      constexpr graph_primitive(edges_initializer edges, std::initializer_list<node_weight_type> nodeWeights)
        requires (!std::is_empty_v<node_weight_type> && !heterogeneous_nodes<graph_primitive>)
        : Connectivity{check_initialization(edges, nodeWeights.size(), edges.size())}
        , Nodes{nodeWeights}
      {}

      template<class... NodeWeights>
        requires heterogeneous_nodes<graph_primitive>
      constexpr graph_primitive(edges_initializer edges, NodeWeights&&... nodeWeights)
        : Connectivity{check_initialization(edges, sizeof...(NodeWeights), edges.size())}
        , Nodes{std::forward<NodeWeights>(nodeWeights)...}
      {}

      template<tree_link_direction dir>
        requires (    !heterogeneous_nodes<graph_primitive>
                   && ((dir == tree_link_direction::symmetric) || is_directed(Connectivity::flavour)))
      constexpr graph_primitive(std::initializer_list<tree_initializer<node_weight_type>> forest, tree_link_direction_constant<dir> tdc)
      {
        for(const auto& tree : forest)
        {
          build_tree(std::numeric_limits<size_type>::max(), tree, tdc);
        }
      }

      template<tree_link_direction dir>
      requires (    !heterogeneous_nodes<graph_primitive>
                 && ((dir == tree_link_direction::symmetric) || is_directed(Connectivity::flavour)))
      constexpr graph_primitive(tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      {
        build_tree(std::numeric_limits<size_type>::max(), tree, tdc);
      }

      constexpr graph_primitive(const graph_primitive&) = default;

      [[nodiscard]]
      constexpr size_type size() const noexcept
      {
        return connectivity_type::size();
      }

      constexpr void swap_nodes(edge_index_type i, edge_index_type j)
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::swap_nodes(i, j);
        }

        Connectivity::swap_nodes(i, j);
      }

      template<class Compare>
      constexpr void sort_nodes(Compare c)
      {
        sort_nodes(edge_index_type{}, Connectivity::order(), std::move(c));
      }

      template<class Compare>
      constexpr void sort_nodes(const edge_index_type first, const edge_index_type last, Compare c)
      {
        sequoia::sort(pseudo_iterator{first, *this}, pseudo_iterator{last, *this}, std::move(c));
      }

      //===============================equality (not isomorphism) operators================================//

      [[nodiscard]]
      friend constexpr bool operator==(const graph_primitive&, const graph_primitive&) noexcept = default;
    protected:
      // Constructors with allocators

      template<class N>
      constexpr static bool enableNodeAllocation{!heterogeneous_nodes<graph_primitive> && !std::is_empty_v<N>};

      template
      <
        alloc EdgeAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
       
        : Connectivity(edgeAlloc)
        , Nodes(nodeAlloc)
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        alloc NodeAllocator
      >
        requires (enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity(edgeAlloc, edgeParitionsAlloc)
        , Nodes(nodeAlloc)
      {}

      template<alloc EdgeAllocator>
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(const EdgeAllocator& edgeAlloc)
        : Connectivity(edgeAlloc)
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator
      >
        requires(!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc)
        : Connectivity(edgeAlloc, edgeParitionsAlloc)
      {}

      template
      <
        alloc EdgeAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc, std::initializer_list<node_weight_type> nodeWeights, const NodeAllocator& nodeAlloc)
        : Connectivity{edges, edgeAlloc}
        , Nodes{nodeWeights, nodeAlloc}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc, std::initializer_list<node_weight_type> nodeWeights, const NodeAllocator& nodeAlloc)
        : Connectivity{edges, edgeAlloc, edgeParitionsAlloc}
        , Nodes{nodeWeights, nodeAlloc}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        class... NodeWeights
      >
        requires heterogeneous_nodes<graph_primitive>
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc, NodeWeights&&... nodeWeights)
        : Connectivity{edges, edgeAlloc, edgeParitionsAlloc}
        , Nodes{std::forward<NodeWeights>(nodeWeights)...}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{edges, edgeAlloc}
        , Nodes(edges.size(), nodeAlloc)
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{edges, edgeAlloc, edgeParitionsAlloc}
        , Nodes(edges.size(), nodeAlloc)
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator
      >
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc)
        : Connectivity{edges, edgeAlloc, edgeParitionsAlloc}
        , Nodes{}
      {}

      template<alloc EdgeAllocator>
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(edges_initializer edges, const EdgeAllocator& edgeAlloc)
        : Connectivity{edges, edgeAlloc}
        , Nodes{}
      {}

      // Copy-like constructors with allocators

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(const graph_primitive& in, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{static_cast<const Connectivity&>(in), edgeAlloc, edgeParitionsAlloc}
        , Nodes{static_cast<const Nodes&>(in), nodeAlloc}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(const graph_primitive& in, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{static_cast<const Connectivity&>(in), edgeAlloc}
        , Nodes{static_cast<const Nodes&>(in), nodeAlloc}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator
      >
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(const graph_primitive& in, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc)
        : Connectivity{static_cast<const Connectivity&>(in), edgeAlloc, edgeParitionsAlloc}
        , Nodes{static_cast<const Nodes&>(in)}
      {}

      template<alloc EdgeAllocator>
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(const graph_primitive& in, const EdgeAllocator& edgeAlloc)
        : Connectivity{static_cast<const Connectivity&>(in), edgeAlloc}
        , Nodes{static_cast<const Nodes&>(in)}
      {}

      constexpr graph_primitive(graph_primitive&&) noexcept = default;

      // Move-like constructors with allocators

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(graph_primitive&& in, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{static_cast<Connectivity&&>(in), edgeAlloc, edgeParitionsAlloc}
        , Nodes{static_cast<Nodes&&>(in), nodeAlloc}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc NodeAllocator
      >
        requires enableNodeAllocation<node_weight_type>
      constexpr graph_primitive(graph_primitive&& in, const EdgeAllocator& edgeAlloc, const NodeAllocator& nodeAlloc)
        : Connectivity{static_cast<Connectivity&&>(in), edgeAlloc}
        , Nodes{static_cast<Nodes&&>(in), nodeAlloc}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc EdgePartitionsAllocator,
        alloc NodeAllocator
      >
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(graph_primitive&& in, const EdgeAllocator& edgeAlloc, const EdgePartitionsAllocator& edgeParitionsAlloc)
        : Connectivity{static_cast<Connectivity&&>(in), edgeAlloc, edgeParitionsAlloc}
        , Nodes{static_cast<Nodes&&>(in)}
      {}

      template
      <
        alloc EdgeAllocator,
        alloc NodeAllocator
      >
        requires (!enableNodeAllocation<node_weight_type>)
      constexpr graph_primitive(graph_primitive&& in, const EdgeAllocator& edgeAlloc)
        : Connectivity{static_cast<Connectivity&&>(in), edgeAlloc}
        , Nodes{static_cast<Nodes&&>(in)}
      {}

      ~graph_primitive() = default;

      constexpr graph_primitive& operator=(graph_primitive&&) noexcept = default;

      constexpr graph_primitive& operator=(const graph_primitive&) = default;

      void swap(graph_primitive& rhs)
        noexcept(noexcept(this->Connectivity::swap(rhs)) && noexcept(this->Nodes::swap(rhs)))
      {
        Connectivity::swap(rhs);
        Nodes::swap(rhs);
      }

      void reserve_nodes(const size_type size)
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::reserve(size);
        }

        Connectivity::reserve_nodes(size);
      }

      [[nodiscard]]
      size_type node_capacity() const noexcept
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          return std::ranges::min(Connectivity::node_capacity(), Nodes::capacity());
        }
        else
        {
          return Connectivity::node_capacity();
        }
      }

      void shrink_to_fit()
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::shrink_to_fit();
        }

        Connectivity::shrink_to_fit();
      }

      template<class... Args>
      size_type add_node(Args&&... args)
      {
        return insert_node(this->order(), std::forward<Args>(args)...);
      }

      template<class... Args>
      size_type insert_node(const size_type pos, Args&&... args)
      {
        insertion_sentinel sentinel{*this, pos};

        return Connectivity::insert_node(insert_node_impl(pos, std::forward<Args>(args)...));
      }

      void erase_node(const size_type node)
      {
        Connectivity::erase_node(node);
        if constexpr (!std::is_empty_v<node_weight_type>) Nodes::erase_node(this->cbegin_node_weights() + node);
      }

      void clear() noexcept
      {
        if constexpr (!std::is_empty_v<node_weight_type>) Nodes::clear();
        Connectivity::clear();
      }

      template<tree_link_direction dir, class... Args>
      size_type insert_node_to_tree(tree_link_direction_constant<dir>, size_type pos, size_type parent, Args&&... args)
      {
        const auto n{insert_node(pos, std::forward<Args>(args)...)};

        if(this->order() > 1)
        {
          if constexpr((dir != tree_link_direction::forward) && is_directed(Connectivity::flavour))
          {
            Connectivity::join(n, parent);
          }

          if constexpr(dir != tree_link_direction::backward)
          {
            Connectivity::join(parent, n);
          }
        }

        return n;
      }

      template<tree_link_direction dir, class... Args>
      size_type add_node_to_tree(tree_link_direction_constant<dir> tldc, size_type parent, Args&&... args)
      {
        return insert_node_to_tree(tldc, this->order(), parent, std::forward<Args>(args)...);
      }
    private:
      class [[nodiscard]] insertion_sentinel
      {
      public:
        insertion_sentinel(graph_primitive& g, size_type index)
          : m_Graph{g}
          , m_NodeIndex{index}
        {}

        ~insertion_sentinel()
        {
          if constexpr(!std::is_empty_v<node_weight_type>)
          {
            if(static_cast<Nodes&>(m_Graph).size() > static_cast<Connectivity&>(m_Graph).order())
              m_Graph.remove_excess_node(m_NodeIndex);
          }
        }
      private:
        graph_primitive& m_Graph{};
        size_type m_NodeIndex{};
      };

      friend insertion_sentinel;

      [[nodiscard]]
      constexpr static const edges_initializer& check_initialization(const edges_initializer& edges, std::size_t numNodes, std::size_t edgeParitions)
      {
        if(numNodes != edgeParitions)
          throw std::logic_error{graph_errors::inconsistent_initialization_message(numNodes, edgeParitions)};

        return edges;
      }

      template<tree_link_direction dir>
        requires (!heterogeneous_nodes<graph_primitive> && !std::is_empty_v<node_weight_type>)
      constexpr void build_tree(size_type root, tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      {
        if(root >= Connectivity::order()) root = add_node(tree.node);

        for(const auto& child : tree.children)
        {
          const auto n{add_node_to_tree(tdc, root, child.node)};
          build_tree(n, child, tdc);
        }
      }

      template<tree_link_direction dir>
        requires (!heterogeneous_nodes<graph_primitive> && std::is_empty_v<node_weight_type>)
      constexpr void build_tree(size_type root, tree_initializer<node_weight_type> tree, tree_link_direction_constant<dir> tdc)
      {
        if(root >= Connectivity::order()) root = add_node();

        for(const auto& child : tree.children)
        {
          const auto n{add_node_to_tree(tdc, root)};
          build_tree(n, child, tdc);
        }
      }

      [[nodiscard]]
      pseudo_iterator begin() noexcept { return {edge_index_type{}, *this}; }

      [[nodiscard]]
      pseudo_iterator end() noexcept { return {Connectivity::order(), *this}; }

      template<class... Args>
      size_type insert_node_impl(const size_type pos, Args&&... args)
      {
        const auto node{std::ranges::min(pos, this->order())};
        if constexpr (!std::is_empty_v<node_weight_type>)
        {
          Nodes::insert_node(this->cbegin_node_weights() + node, std::forward<Args>(args)...);
        }

        return node;
      }

      void remove_excess_node(size_type index)
      {
        if(!std::is_empty_v<node_weight_type>)
        {
          Nodes::erase_node(this->cbegin_node_weights() + index);
        }
      }
    };
  }
}
