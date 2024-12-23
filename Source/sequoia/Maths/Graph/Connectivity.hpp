////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation for a partitioned sequence of edges, which represents
    a graph's connectivity.

    Graphs consist of edges and nodes. If the nodes do not possess a weight,
    they may be treated entirely implicitly. A partitioned sequence of edges,
    where each partition corresponds to a node and each edges records
    the other node to which it attaches, suffices to competely specify the
    graph. This data structure will be referred to as the `connectivity`.
    For graphs with weighted nodes, the same `connectivity` data structure
    may be used, but now in conjunction with a separate structure to record
    the node weights.

 */

#include "sequoia/Core/DataStructures/DataStructuresTypeTraits.hpp"
#include "sequoia/Maths/Graph/GraphDetails.hpp"
#include "sequoia/Maths/Graph/GraphErrors.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"
#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/Core/ContainerUtilities/AssignmentUtilities.hpp"
#include "sequoia/Core/Object/HandlerTraits.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <limits>
#include <stdexcept>
#include <ranges>

namespace sequoia
{
  namespace data_structures
  {
    template <class, class, class> class partitioned_sequence;
  }

  namespace maths
  {
    namespace graph_impl
    {
      template<class EdgeType, class EdgeStorageType>
      class edge_maker
      {
        using edge_type           = EdgeType;
        using edge_weight_type    = typename edge_type::weight_type;
        using edge_index_type     = typename edge_type::index_type;
        using edge_storage_type   = EdgeStorageType;
        using const_edge_iterator = typename edge_storage_type::const_partition_iterator;
        using size_type           = typename edge_storage_type::size_type;

        struct data
        {
          const edge_weight_type* m_WeightPtr{};
          edge_index_type node_index{}, edge_index{};
        };

        edge_storage_type& m_Storage;
        std::vector<data> m_WeightMap;
      public:
        explicit edge_maker(edge_storage_type& storage)
          : m_Storage{storage}
        {}

        void operator()(const size_type node, const_edge_iterator first, const_edge_iterator i)
        {
          auto weightPtr{&i->weight()};
          auto found{std::ranges::lower_bound(m_WeightMap, weightPtr, [](const edge_weight_type* lhs, const edge_weight_type* rhs) { return lhs < rhs; }, [](const data& d) { return d.m_WeightPtr; })};
          if((found == m_WeightMap.end()) || (found->m_WeightPtr != weightPtr))
          {
            auto appender{[=, this](auto e){
                   m_Storage.push_back_to_partition(node, std::move(e));
                   m_WeightMap.emplace(found, &i->weight(), node, static_cast<edge_index_type>(std::ranges::distance(first, i)));
                 }
            };

            appender(edge_type{i->target_node(), i->weight()});
          }
          else
          {
            m_Storage.push_back_to_partition(node, i->target_node(), *(m_Storage.cbegin_partition(found->node_index) + found->edge_index));
          }
        }
      };

      template<std::input_or_output_iterator Iter>
      class [[nodiscard]] weight_sentinel
      {
      public:
        using edge_weight_type = std::remove_cvref_t<decltype(std::declval<Iter>()->weight())>;

        template<class Fn1, class Fn2, class... Args1>
        constexpr weight_sentinel(Iter i, Fn1 fn1, Fn2 fn2, Args1&&... args1)
          : m_Iter{i}
          , m_OldEdgeWeight{m_Iter->weight()}
        {
          auto partnerIter{fn1(m_Iter, std::forward<Args1>(args1)...)};
          m_PartiallyComplete = true;

          fn2(m_Iter, partnerIter);
          m_PartiallyComplete = false;
        }

        constexpr ~weight_sentinel()
        {
          if(m_PartiallyComplete)
          {
            m_Iter->weight(std::move(m_OldEdgeWeight));
          }
        }
      private:
        bool m_PartiallyComplete{};
        Iter m_Iter;
        edge_weight_type m_OldEdgeWeight;
      };

      template<class Edges>
      class [[nodiscard]] join_sentinel
      {
      public:
        using edge_index_type = typename Edges::index_type;

        join_sentinel(Edges& e, const edge_index_type node1, const edge_index_type pos)
          : m_Edges{e}
          , m_Node1{node1}
          , m_Pos{pos}
          , m_InitialSize{e.size()}
        {}

        ~join_sentinel()
        {
          if(m_Edges.size() == m_InitialSize)
          {
            m_Edges.erase_from_partition(m_Node1, m_Pos);
          }
        }
      private:
        Edges& m_Edges;
        edge_index_type m_Node1{}, m_Pos{};
        std::size_t m_InitialSize{};
      };

      struct edge_comparer
      {
        template<class Edge>
        [[nodiscard]]
        constexpr bool operator()(const Edge& e1, const Edge& e2) const noexcept
        {
          using edge_weight_type = typename Edge::weight_type;
          constexpr bool sort_weights{!std::is_empty_v<edge_weight_type> && std::totally_ordered<edge_weight_type>};

          if constexpr(!sort_weights)
          {
            return e1.target_node() < e2.target_node();
          }
          else
          {
            return (e1.target_node() == e2.target_node()) ? e1.weight() < e2.weight() : e1.target_node() < e2.target_node();
          }
        }
      };
    }

    struct partitions_allocator_tag{};

    /*! \brief Graph connectivity_base, used as a building block for concrete graphs.
    
        This class is flexible, allowing for representations of many different flavours
        of connectivity. It is designed for inheritance by concrete graphs; therefore
        various methods, including the destructor, are protected. Both static and
        dynamic graphs are supported; for the purposes of the latter, the relevant
        protected methods of `connectivity_base` are allocator-aware.
     */
    template<graph_flavour Flavour, class EdgeStorage>
    class connectivity_base
    {
      friend struct sequoia::assignment_helper;
    public:
      using edge_storage_type = EdgeStorage;

      using edge_type           = typename edge_storage_type::value_type;
      using edge_weight_type    = typename edge_type::weight_type;
      using edge_meta_data_type = typename edge_type::meta_data_type;
      using edge_index_type     = typename edge_type::index_type;
      using edge_init_type      = edge_init_type_generator_t<edge_type>;
      using edges_initializer   = std::initializer_list<std::initializer_list<edge_init_type>>;

      using size_type                   = typename edge_storage_type::size_type;
      using const_edge_iterator         = typename edge_storage_type::const_partition_iterator;
      using const_reverse_edge_iterator = typename edge_storage_type::const_reverse_partition_iterator;
      using const_edges_range           = std::ranges::subrange<const_edge_iterator>;

      static_assert(std::is_unsigned_v<edge_index_type>);

      constexpr static auto npos{std::numeric_limits<edge_index_type>::max()};
      constexpr static graph_flavour flavour{Flavour};

      constexpr connectivity_base() = default;

      constexpr connectivity_base(std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : m_Edges{make_edges(edges)}
      {}

      constexpr connectivity_base(const connectivity_base& other)
        : m_Edges{copy_edges(other)}
      {}

      [[nodiscard]]
      constexpr size_type order() const noexcept { return m_Edges.num_partitions(); }

      [[nodiscard]]
      constexpr size_type size() const noexcept
      {
        return !is_directed(flavour) ? m_Edges.size() / 2 : m_Edges.size();
      }

      [[nodiscard]]
      constexpr const_edge_iterator cbegin_edges(const edge_index_type node) const
      {
        graph_errors::check_node_index_range("cbegin_edges", order(), node);

        return m_Edges.cbegin_partition(node);
      }

      [[nodiscard]]
      constexpr const_edge_iterator cend_edges(const edge_index_type node) const
      {
        graph_errors::check_node_index_range("cend_edges", order(), node);

        return m_Edges.cend_partition(node);
      }

      [[nodiscard]]
      constexpr const_reverse_edge_iterator crbegin_edges(const edge_index_type node) const
      {
        graph_errors::check_node_index_range("crbegin_edges", order(), node);

        return m_Edges.crbegin_partition(node);
      }

      [[nodiscard]]
      constexpr const_reverse_edge_iterator crend_edges(const edge_index_type node) const
      {
        graph_errors::check_node_index_range("crend_edges", order(), node);

        return m_Edges.crend_partition(node);
      }

      [[nodiscard]]
      constexpr const_edges_range cedges(const edge_index_type node) const
      {
        graph_errors::check_node_index_range("cedges", order(), node);

        return {m_Edges.cbegin_partition(node), m_Edges.cend_partition(node)};
      }

      template<class... Args>
        requires initializable_from<edge_weight_type, Args...>
      constexpr void set_edge_weight(const_edge_iterator citer, Args&&... args)
      {
        if constexpr(!shared_weight_v && !is_directed(flavour))
        {
          auto partnerSetter{
            [this](const_edge_iterator iter, auto&&... args){
              return this->set_partner_edge_weight(iter, std::forward<decltype(args)>(args)...);
            }
          };

          auto sourceSetter{
            [this](edge_iterator iter, const_edge_iterator partnerIter){
              this->set_source_edge_weight(iter, partnerIter->weight());
            }
          };

          graph_impl::weight_sentinel sentinel{to_edge_iterator(citer), partnerSetter, sourceSetter, std::forward<Args>(args)...};
        }
        else
        {
          set_source_edge_weight(to_edge_iterator(citer), std::forward<Args>(args)...);
        }
      }

      template<class... Args>
        requires initializable_from<edge_weight_type, Args...>
      constexpr void set_edge_weight(const_reverse_edge_iterator criter, Args&&... args)
      {
        set_edge_weight(to_const_edge_iterator(criter), std::forward<Args>(args)...);
      }

      template<std::invocable<edge_weight_type&> Fn>
        requires (!std::is_empty_v<edge_weight_type>)
      constexpr std::invoke_result_t<Fn, edge_weight_type&> mutate_edge_weight(const_edge_iterator citer, Fn fn)
      {
        if constexpr(!shared_weight_v && !is_directed(flavour))
        {
          mutate_partner_edge_weight(citer, fn);
        }

        return mutate_source_edge_weight(citer, fn);
      }

      template<std::invocable<edge_weight_type&> Fn>
        requires (!std::is_empty_v<edge_weight_type>)
      constexpr std::invoke_result_t<Fn, edge_weight_type&> mutate_edge_weight(const_reverse_edge_iterator criter, Fn fn)
      {
        return mutate_edge_weight(to_const_edge_iterator(criter), std::move(fn));
      }

      template<class... Args>
        requires initializable_from<edge_meta_data_type, Args...>
      constexpr void set_edge_meta_data(const_edge_iterator citer, Args&&... args)
      {
        to_edge_iterator(citer)->meta_data(std::forward<Args>(args)...);
      }

      template<class... Args>
        requires initializable_from<edge_meta_data_type, Args...>
      constexpr void set_edge_meta_data(const_reverse_edge_iterator criter, Args&&... args)
      {
        set_edge_meta_data(to_const_edge_iterator(criter), std::forward<Args>(args)...);
      }


      template<std::invocable<edge_meta_data_type&> Fn>
        requires (!std::is_empty_v<edge_meta_data_type>)
      constexpr std::invoke_result_t<Fn, edge_meta_data_type&> mutate_edge_meta_data(const_edge_iterator citer, Fn fn)
      {
        return to_edge_iterator(citer)->mutate_meta_data(std::move(fn));
      }


      template<std::invocable<edge_meta_data_type&> Fn>
        requires (!std::is_empty_v<edge_meta_data_type>)
      constexpr std::invoke_result_t<Fn, edge_meta_data_type&> mutate_edge_meta_data(const_reverse_edge_iterator criter, Fn fn)
      {
        return mutate_edge_meta_data(to_const_edge_iterator(criter), std::move(fn));
      }

      //===============================equality (not isomorphism) operators================================//

      [[nodiscard]]
      friend constexpr bool operator==(const connectivity_base& lhs, const connectivity_base& rhs) noexcept
        requires deep_equality_comparable<edge_type>
      {
        return lhs.m_Edges == rhs.m_Edges;
      }
    protected:
      using edge_iterator         = typename edge_storage_type::partition_iterator;
      using reverse_edge_iterator = typename edge_storage_type::reverse_partition_iterator;
      using edges_range = std::ranges::subrange<edge_iterator>;

      template<alloc... Allocators>
        requires (sizeof...(Allocators) > 0)
      constexpr connectivity_base(const Allocators&... as)
        : m_Edges(as...)
      {}

      template<alloc... Allocators>
        requires (sizeof...(Allocators) > 0)
      constexpr connectivity_base(edges_initializer edges, const Allocators&... as)
        : m_Edges{make_edges(edges, as...)}
      {}

      template<alloc... Allocators>
        requires (sizeof...(Allocators) > 0)
      constexpr connectivity_base(const connectivity_base& c, const Allocators&... as)
        : m_Edges{copy_edges(c, as...)}
      {}

      constexpr connectivity_base(connectivity_base&&) noexcept = default;

      template<alloc... Allocators>
      constexpr connectivity_base(connectivity_base&& c, const Allocators&... as)
        : m_Edges{std::move(c.m_Edges), as...}
      {}

      ~connectivity_base() = default;

      constexpr connectivity_base& operator=(connectivity_base&&) = default;

      constexpr connectivity_base& operator=(const connectivity_base& in)
      {
        if(&in != this)
        {
          auto allocGetter{
            []([[maybe_unused]] const connectivity_base& in){
              if constexpr(has_allocator_type_v<edge_storage_type>)
              {
                return in.m_Edges.get_allocator();
              }
            }
          };

          auto partitionsAllocGetter{
            []([[maybe_unused]] const connectivity_base& in){
              if constexpr(has_partitions_allocator<edge_storage_type>)
              {
                return in.m_Edges.get_partitions_allocator();
              }
            }
          };

          assignment_helper::assign(*this, in, allocGetter, partitionsAllocGetter);
        }

        return *this;
      }

      template<alloc... Allocs>
        requires ((sizeof...(Allocs) > 0)
      && (std::allocator_traits<Allocs>::propagate_on_container_copy_assignment::value && ...))
        void reset(const Allocs&... allocs)
      {
        m_Edges.reset(allocs...);
      }

      void swap(connectivity_base& rhs)
        noexcept(noexcept(std::ranges::swap(this->m_Edges, rhs.m_Edges)))
      {
        std::ranges::swap(m_Edges, rhs.m_Edges);
      }

      // TO DO: change semantics to non-throwing?
      constexpr void swap_edges(edge_index_type node, edge_index_type i, edge_index_type j)
      {
        graph_errors::check_edge_swap_indices(node, i, j, static_cast<std::size_t>(std::ranges::distance(cedges(node))));

        std::ranges::iter_swap(begin_edges(node) + i, begin_edges(node) + j);
      }

      // TO DO: change semantics to non-throwing?
      constexpr void swap_nodes(edge_index_type i, edge_index_type j)
      {
        graph_errors::check_node_index_range("swap_nodes", order(), i, j);

        if(i == j) return;

        auto fixUp{
          [this, i, j](edge_index_type n){
            for(auto& e : edges(n))
            {
              if     (e.target_node() == i) e.target_node(j);
              else if(e.target_node() == j) e.target_node(i);
            }
          }
        };

        if constexpr(!is_directed(flavour))
        {
          // TO DO: use concat_view when available
          auto getTarget{[](const auto& e){ return e.target_node(); }};
          auto targetEdges{std::views::transform(edges(i), getTarget) | std::ranges::to<std::vector>()};
          std::ranges::transform(edges(j), std::back_inserter(targetEdges), getTarget);
          std::ranges::sort(targetEdges);
          auto duplicates{std::ranges::unique(targetEdges)};

          for(auto iter{targetEdges.begin()}; iter != duplicates.begin(); ++iter)
          {
            fixUp(*iter);
          }
        }
        else
        {
          for(edge_index_type n{}; n < static_cast<edge_index_type>(order()); ++n)
          {
            fixUp(n);
          }
        }

        m_Edges.swap_partitions(i, j);
      }

      [[nodiscard]]
      auto get_edge_allocator() const
      {
        return m_Edges.get_allocator();
      }

      [[nodiscard]]
      auto get_edge_allocator(partitions_allocator_tag) const
        requires has_partitions_allocator<edge_storage_type>
      {
        return m_Edges.get_partitions_allocator();
      }

      void reserve_nodes(const size_type size)
      {
        m_Edges.reserve_partitions(size);
      }

      [[nodiscard]]
      size_type node_capacity() const noexcept
      {
        return m_Edges.num_partitions_capacity();
      }

      void reserve_edges(const edge_index_type partition, const edge_index_type size)
        requires graph_impl::has_reservable_partitions<edge_storage_type>
      {
        m_Edges.reserve_partition(partition, size);
      }

      void reserve_edges(const edge_index_type size)
        requires (!graph_impl::has_reservable_partitions<edge_storage_type>)
      {
        m_Edges.reserve(size);
      }

      [[nodiscard]]
      size_type edges_capacity(const edge_index_type partition) const
        requires graph_impl::has_reservable_partitions<edge_storage_type>
      {
        return m_Edges.partition_capacity(partition);
      }

      [[nodiscard]]
      size_type edges_capacity() const noexcept
        requires (!graph_impl::has_reservable_partitions<edge_storage_type>)
      {
        return m_Edges.capacity();
      }

      void shrink_to_fit()
      {
        m_Edges.shrink_to_fit();
      }

      // TO DO: Consider interaction of throwing semantics with that of edge storage class
      [[nodiscard]]
      constexpr edge_iterator begin_edges(const edge_index_type node)
      {
        graph_errors::check_node_index_range("begin_edges", order(), node);

        return m_Edges.begin_partition(node);
      }

      [[nodiscard]]
      constexpr edge_iterator end_edges(const edge_index_type node)
      {
        graph_errors::check_node_index_range("end_edges", order(), node);

        return m_Edges.end_partition(node);
      }

      [[nodiscard]]
      constexpr const_edge_iterator begin_edges(const edge_index_type node) const
      {
          graph_errors::check_node_index_range("begin_edges", order(), node);

          return m_Edges.begin_partition(node);
      }

      [[nodiscard]]
      constexpr const_edge_iterator end_edges(const edge_index_type node) const
      {
          graph_errors::check_node_index_range("end_edges", order(), node);

          return m_Edges.end_partition(node);
      }

      [[nodiscard]]
      constexpr reverse_edge_iterator rbegin_edges(const edge_index_type node)
      {
        graph_errors::check_node_index_range("rbegin_edges", order(), node);

        return m_Edges.rbegin_partition(node);
      }

      [[nodiscard]]
      constexpr reverse_edge_iterator rend_edges(const edge_index_type node)
      {
        graph_errors::check_node_index_range("rend_edges", order(), node);

        return m_Edges.rend_partition(node);
      }

      [[nodiscard]]
      constexpr const_reverse_edge_iterator rbegin_edges(const edge_index_type node) const
      {
          graph_errors::check_node_index_range("rbegin_edges", order(), node);

          return m_Edges.rbegin_partition(node);
      }

      [[nodiscard]]
      constexpr const_reverse_edge_iterator rend_edges(const edge_index_type node) const
      {
          graph_errors::check_node_index_range("rend_edges", order(), node);

          return m_Edges.rend_partition(node);
      }

      [[nodiscard]]
      constexpr edges_range edges(const edge_index_type node) { return {begin_edges(node), end_edges(node)}; }

      void add_node()
      {
        m_Edges.add_slot();
      }

      size_type insert_node(const size_type node)
      {
        m_Edges.insert_slot(node);
        fix_edge_data(node,
          [](const auto targetNode, const auto node) { return targetNode >= node; },
          [](const auto index) { return index + 1; });

        return node;
      }

      void erase_node(const size_type node)
      {
        graph_errors::check_node_index_range("erase_node", order(), node);

        if constexpr(!is_directed(flavour))
        {
          std::vector<size_type> partitionsToVisit{};

          for(const auto& edge : m_Edges.partition(node))
          {
            const auto target{edge.target_node()};
            if(target != node) partitionsToVisit.push_back(target);
          }

          std::ranges::sort(partitionsToVisit);
          auto duplicates{std::ranges::unique(partitionsToVisit)};

          for(const auto partition : std::ranges::subrange{partitionsToVisit.begin(), duplicates.begin()})
          {
            auto fn{[node](const edge_type& e) { return e.target_node() == node; }};

            if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
            {
              // TO DO: consider reversing iteration
              for(auto iter{m_Edges.begin_partition(partition)}; iter != m_Edges.end_partition(partition);)
              {
                if(fn(*iter))
                {
                  iter = m_Edges.erase_from_partition(iter);
                  decrement_comp_indices(iter, m_Edges.end_partition(partition), 1);
                }
                else
                {
                  ++iter;
                }
              }
            }
            else
            {
              erase_from_partition_if(partition, fn);
            }
          }
        }
        else
        {
          for(size_type i{}; i < m_Edges.num_partitions(); ++i)
          {
            if(i == node) continue;
            erase_from_partition_if(i, [node](const edge_type& e) {
              return (e.target_node() == node);
              });
          }
        }

        m_Edges.erase_slot(node);
        fix_edge_data(node,
          [](const auto targetNode, const auto node) { return targetNode > node; },
          [](const auto index) { return index - 1; });
      }

      template<class... Args>
        requires (std::is_empty_v<edge_meta_data_type>&& initializable_from<edge_weight_type, Args...> && (is_directed(flavour) || std::is_copy_constructible_v<edge_type>))
      void join(const edge_index_type node1, const edge_index_type node2, Args&&... args)
      {
        graph_errors::check_node_index_range("join", order(), node1, node2);

        add_to_partition(node1, node2, std::forward<Args>(args)...);

        if constexpr(!is_directed(flavour))
        {
          reciprocal_join(node1, node2);
        }
      }

      template<class... Args>
        requires (!std::is_empty_v<edge_meta_data_type>&& initializable_from<edge_weight_type, Args...> && (is_directed(flavour) || std::is_copy_constructible_v<edge_type>))
      void join(const edge_index_type node1, const edge_index_type node2, edge_meta_data_type meta1, edge_meta_data_type meta2, Args&&... args)
      {
        graph_errors::check_node_index_range("join", order(), node1, node2);

        add_to_partition(node1, node2, meta1, std::forward<Args>(args)...);

        if constexpr(!is_directed(flavour))
        {
          reciprocal_join(node1, node2, std::move(meta2));
        }
      }

      template<class... Args>
        requires (std::is_empty_v<edge_meta_data_type> && initializable_from<edge_weight_type, Args...>&& is_embedded(flavour) && std::is_copy_constructible_v<edge_type>)
      std::pair<const_edge_iterator, const_edge_iterator>
        insert_join(const_edge_iterator citer1, const_edge_iterator citer2, Args&&... args)
      {
        const auto node1{citer1.partition_index()}, node2{citer2.partition_index()};
        const auto dist2{static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(node2), citer2))};
        if(node1 == node2) return insert_join(citer1, dist2, std::forward<Args>(args)...);

        citer1 = insert_to_partition(citer1, node2, dist2, std::forward<Args>(args)...);
        return insert_reciprocal_join(citer1, cbegin_edges(node2) + dist2);
      }

      template<class... Args>
        requires (!std::is_empty_v<edge_meta_data_type>, initializable_from<edge_weight_type, Args...>&& is_embedded(flavour) && std::is_copy_constructible_v<edge_type>)
      std::pair<const_edge_iterator, const_edge_iterator>
        insert_join(const_edge_iterator citer1, const_edge_iterator citer2, edge_meta_data_type meta1, edge_meta_data_type meta2, Args&&... args)
      {
        const auto node1{citer1.partition_index()}, node2{citer2.partition_index()};
        const auto dist2{static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(node2), citer2))};
        if(node1 == node2) return insert_join(citer1, dist2, meta1, meta2, std::forward<Args>(args)...);

        citer1 = insert_to_partition(citer1, node2, dist2, meta1, std::forward<Args>(args)...);
        return insert_reciprocal_join(citer1, cbegin_edges(node2) + dist2, meta2);
      }

      template<class... Args>
        requires (std::is_empty_v<edge_meta_data_type> && initializable_from<edge_weight_type, Args...>&& is_embedded(flavour) && std::is_copy_constructible_v<edge_type>)
      std::pair<const_edge_iterator, const_edge_iterator>
        insert_join(const_edge_iterator citer1, const edge_index_type pos2, Args&&... args)
      {
        const auto node{citer1.partition_index()};
        graph_errors::check_edge_insertion_index("insert_join", node, std::ranges::distance(cedges(node)) + 1, pos2);

        citer1 = insert_to_partition(citer1, node, pos2, std::forward<Args>(args)...);

        return insert_reciprocal_join(citer1, pos2);
      }

      template<class... Args>
        requires (!std::is_empty_v<edge_meta_data_type>, initializable_from<edge_weight_type, Args...>&& is_embedded(flavour) && std::is_copy_constructible_v<edge_type>)
      std::pair<const_edge_iterator, const_edge_iterator>
        insert_join(const_edge_iterator citer1, const edge_index_type pos2, edge_meta_data_type meta1, edge_meta_data_type meta2, Args&&... args)
      {
        const auto node{citer1.partition_index()};
        graph_errors::check_edge_insertion_index("insert_join", node, std::ranges::distance(cedges(node)) + 1, pos2);

        citer1 = insert_to_partition(citer1, node, pos2, meta1, std::forward<Args>(args)...);

        return insert_reciprocal_join(citer1, pos2, meta2);
      }

      void erase_edge(const_edge_iterator citer)
      {
        // TO DO: ensure strong exception guarantee (maybe just insist on
        // noexcept move assignment for m_Edges

        if(!order() || (citer == cend_edges(citer.partition_index()))) return;

        if constexpr (!is_directed(flavour))
        {
          const auto source{citer.partition_index()};
          const auto partner{citer->target_node()};

          auto pred{
            [=](const edge_type& potentialPartner){
              if(potentialPartner.target_node() == source)
              {
                if constexpr(std::is_empty_v<edge_weight_type>)
                  return true;
                else if constexpr(shared_weight_v)
                  return std::addressof(citer->weight()) == std::addressof(potentialPartner.weight());
                else if constexpr(std::is_empty_v<edge_meta_data_type>)
                  return citer->weight() == potentialPartner.weight();
                else
                  static_assert(dependent_false<edge_type>::value);
              }

              return false;
            }
          };

          if(source != partner)
          {
            if constexpr (edge_type::flavour == edge_flavour::partial_embedded)
            {
              const auto partnerLocalIndex{citer->complementary_index()};
              citer = m_Edges.erase_from_partition(citer);
              decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(source), 1);

              auto partnerIter{m_Edges.erase_from_partition(partner, partnerLocalIndex)};
              decrement_comp_indices(partnerIter, m_Edges.end_partition(partner), 1);
            }
            else
            {
              auto found{std::ranges::find_if(cedges(partner), pred)};

              if(found == cend_edges(partner))
                throw std::logic_error{graph_errors::erase_edge_error(partner, {source, static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(source), citer))})};

              const auto partnerDist{std::ranges::distance(cbegin_edges(partner), found)};

              m_Edges.erase_from_partition(citer);
              m_Edges.erase_from_partition(cbegin_edges(partner) + partnerDist);
            }
          }
          else
          {
            if constexpr (edge_type::flavour == edge_flavour::partial_embedded)
            {
              const auto compIndex{citer->complementary_index()};
              const auto pos{std::ranges::distance(cbegin_edges(source), citer)};
              const auto separation{std::ranges::distance(citer, cbegin_edges(source) + compIndex)};
              if((separation == 1) || (separation == -1))
              {
                const auto delta{separation == 1 ? 0 : -1};
                citer = m_Edges.erase_from_partition(citer+delta, citer+(2+delta));

                decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(source), 2);
              }
              else
              {
                const bool negativeSeparation{separation < 0};

                auto erase{
                  [this, source](auto index){
                    auto i{m_Edges.erase_from_partition(m_Edges.begin_partition(source) + index)};
                    decrement_comp_indices(i, m_Edges.end_partition(source), 1);
                  }
                };

                erase(negativeSeparation ? pos : compIndex);
                erase(negativeSeparation ? compIndex : pos);
              }
            }
            else
            {
              auto found{std::ranges::find_if(cbegin_edges(source), citer, pred)};
              if(found == citer) found = std::ranges::find_if(std::ranges::next(citer), cend_edges(source), pred);

              if(found == cend_edges(source))
                throw std::logic_error{graph_errors::erase_edge_error(source, {source, static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(source), citer))})};

              auto dist{std::ranges::distance(cbegin_edges(source), citer)};
              if(std::ranges::distance(cbegin_edges(source), found) < dist) --dist;

              m_Edges.erase_from_partition(found);
              m_Edges.erase_from_partition(cbegin_edges(source) + dist);
            }
          }
        }
        else
        {
          m_Edges.erase_from_partition(citer);
        }
      }

      void clear() noexcept
      {
        m_Edges.clear();
      }

      template<class Comp, class Proj = std::identity>
      constexpr void sort_edges(const_edge_iterator begin, const_edge_iterator end, Comp comp, Proj proj = {})
        requires ((edge_type::flavour == edge_flavour::partial) && std::sortable<edge_iterator, Comp, Proj>)
      {
        if(begin.partition_index() != end.partition_index()) return;

        std::ranges::sort(to_edge_iterator(begin), to_edge_iterator(end), std::move(comp), std::move(proj));
      }

      template<class Comp, class Proj = std::identity>
      constexpr void sort_edges(const_edges_range r, Comp comp, Proj proj = {})
        requires ((edge_type::flavour == edge_flavour::partial) && std::sortable<edge_iterator, Comp, Proj>)
      {
        sort_edges(r.begin(), r.end(), std::move(comp), std::move(proj));
      }

      template<class Comp, class Proj = std::identity>
      constexpr void stable_sort_edges(const_edge_iterator begin, const_edge_iterator end, Comp comp, Proj proj = {})
        requires ((edge_type::flavour == edge_flavour::partial) && merge_sortable<edge_iterator, Comp, Proj>)
      {
        if(begin.partition_index() != end.partition_index()) return;

        sequoia::stable_sort(to_edge_iterator(begin), to_edge_iterator(end), std::move(comp), std::move(proj));
      }

      template<class Comp, class Proj = std::identity>
      constexpr void stable_sort_edges(const_edges_range r, Comp comp, Proj proj = {})
        requires ((edge_type::flavour == edge_flavour::partial) && merge_sortable<edge_iterator, Comp, Proj>)
      {
        stable_sort_edges(r.begin(), r.end(), std::move(comp), std::move(proj));
      }

    private:
      constexpr static bool direct_init_v{std::is_same_v<edge_type, edge_init_type>};
      constexpr static bool direct_copy_v{direct_init_v};
      constexpr static bool shared_weight_v{graph_impl::has_shared_weight_v<edge_type>};

      // private data
      edge_storage_type m_Edges;

      // helper methods

      template<class Pred>
      void erase_from_partition_if(const edge_index_type partitionIndex, Pred pred)
      {
        auto discarded{std::ranges::remove_if(m_Edges.begin_partition(partitionIndex), m_Edges.end_partition(partitionIndex), pred)};
        m_Edges.erase_from_partition(discarded.begin(), discarded.end());
      }

      template<alloc... Allocators>
      [[nodiscard]]
      constexpr edge_storage_type make_edges(edges_initializer edges, const Allocators&... as)
      {
        return process_edges(validate(preprocess(edges)), as...);
      }

      [[nodiscard]]
      constexpr static const edges_initializer& preprocess(const edges_initializer& edges)
        requires (is_embedded(flavour) || is_directed(flavour))
      {
        return edges;
      }

      [[nodiscard]]
      constexpr static edge_storage_type preprocess(edges_initializer edges)
        requires (direct_init_v && !is_embedded(flavour) && !is_directed(flavour))
      {
        return preprocess(edge_storage_type{edges});
      }

      [[nodiscard]]
      constexpr static auto preprocess(edges_initializer edges)
        requires (!direct_init_v && !is_embedded(flavour) && !is_directed(flavour))
      {
        using namespace data_structures;
        using sequence_t = partitioned_sequence<edge_init_type>;

        return preprocess(sequence_t{edges});
      }

      template<class IntermediateEdges>
      [[nodiscard]]
      constexpr static IntermediateEdges preprocess(IntermediateEdges edges)
        requires (!is_embedded(flavour) && !is_directed(flavour))
      {
        constexpr bool clusterEdges{!std::is_empty_v<edge_weight_type> && !std::totally_ordered<edge_weight_type>};

        for(edge_index_type i{}; i < edges.num_partitions(); ++i)
        {
          sequoia::stable_sort(edges.begin_partition(i), edges.end_partition(i), graph_impl::edge_comparer{});

          if constexpr(clusterEdges)
          {
            auto lowerIter{edges.begin_partition(i)}, upperIter{edges.begin_partition(i)};
            while(lowerIter != edges.end_partition(i))
            {
              upperIter = std::ranges::find_if_not(upperIter, edges.end_partition(i), [target{lowerIter->target_node()}](const auto& wt) { return target == wt.target_node(); });

              sequoia::cluster(lowerIter, upperIter, [](const auto& e1, const auto& e2) {
                return e1.weight() == e2.weight();
                });

              lowerIter = upperIter;
            }
          }
        }

        return edges;
      }

      [[nodiscard]]
      constexpr static const edges_initializer& validate(const edges_initializer& edges)
        requires (is_embedded(flavour))
      {
        for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
        {
          const auto nodeIndex{static_cast<std::size_t>(std::ranges::distance(edges.begin(), nodeEdgesIter))};
          const auto& nodeEdges{*nodeEdgesIter};
          for(auto edgeIter{nodeEdges.begin()}; edgeIter != nodeEdges.end(); ++edgeIter)
          {
            const auto edgeIndex{static_cast<std::size_t>(std::ranges::distance(nodeEdges.begin(), edgeIter))};
            const auto& edge{*edgeIter};
            const auto target{edge.target_node()};

            graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "target", edges.size(), target);
            const auto compIndex{edge.complementary_index()};

            const bool doValidate{
              [&]() {
                if constexpr(!is_directed(flavour)) return true;
                else return (edge.target_node() != nodeIndex) || (edge.source_node() == edge.target_node());
              }()
            };

            if(doValidate)
            {
              auto targetEdgesIter{edges.begin() + target};
              graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "complementary", targetEdgesIter->size(), compIndex);

              if((target == nodeIndex) && (compIndex == edgeIndex))
              {
                throw std::logic_error{graph_errors::self_referential_error({nodeIndex, edgeIndex}, target, compIndex)};
              }
              else if(const auto& targetEdge{*(targetEdgesIter->begin() + compIndex)}; targetEdge.complementary_index() != edgeIndex)
              {
                throw std::logic_error{graph_errors::reciprocated_error_message({nodeIndex, edgeIndex}, "complementary", targetEdge.complementary_index(), edgeIndex)};
              }
              else
              {
                if constexpr(!is_directed(flavour))
                {
                  graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "target", targetEdge.target_node(), nodeIndex);
                }
                else
                {
                  graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "target", targetEdge.target_node(), target);
                  graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "source", targetEdge.source_node(), edge.source_node());

                  if constexpr(is_embedded(flavour))
                  {
                    graph_errors::check_inversion_consistency(nodeIndex, {edgeIndex, edge.inverted()}, {compIndex, targetEdge.inverted()});
                  }
                }

                if constexpr(!std::is_empty_v<edge_weight_type>)
                {
                  if(edge.weight() != targetEdge.weight())
                    throw std::logic_error{graph_errors::mismatched_weights_message("process_complementary_edges", {nodeIndex, edgeIndex})};
                }
              }
            }

            if constexpr(is_directed(flavour))
            {
              const auto source{edge.source_node()};
              graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "source", edges.size(), source);
              graph_errors::check_embedded_edge(nodeIndex, source, target);

              if((edge.target_node() == nodeIndex) && (edge.source_node() != edge.target_node()))
              {
                auto sourceEdgesIter{edges.begin() + source};
                graph_errors::check_edge_index_range("process_complementary_edges", {nodeIndex, edgeIndex}, "complementary", sourceEdgesIter->size(), compIndex);

                const auto& sourceEdge{*(sourceEdgesIter->begin() + compIndex)};
                graph_errors::check_reciprocated_index({nodeIndex, edgeIndex}, "target", sourceEdge.target_node(), nodeIndex);
              }
            }
          }
        }

        return edges;
      }

      [[nodiscard]]
      constexpr static const edges_initializer& validate(const edges_initializer& edges)
        requires (!is_embedded(flavour) && is_directed(flavour))
      {
        for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
        {
          const auto nodeIndex{static_cast<std::size_t>(std::ranges::distance(edges.begin(), nodeEdgesIter))};
          const auto& nodeEdges{*nodeEdgesIter};
          for(auto edgeIter{nodeEdges.begin()}; edgeIter != nodeEdges.end(); ++edgeIter)
          {
            const auto edgeIndex{static_cast<std::size_t>(std::ranges::distance(nodeEdges.begin(), edgeIter))};
            const auto& edge{*edgeIter};

            const auto target{edge.target_node()};
            graph_errors::check_edge_index_range("process_edges", {nodeIndex, edgeIndex}, "target", edges.size(), target);
          }
        }

        return edges;
      }


      template<class Edges>
      using partition_iterator_range = std::ranges::subrange<typename Edges::const_partition_iterator>;

      template<class IntermediateEdges>
      [[nodiscard]]
      constexpr static decltype(auto) validate(IntermediateEdges&& edges)
        requires (!is_embedded(flavour) && !is_directed(flavour))
      {
        using range_t = partition_iterator_range<IntermediateEdges>;
        using namespace graph_errors;

        visit_edges(
          edges,
          []() {},
          [](edge_index_type i, range_t hostRange) {
            if(std::ranges::distance(hostRange) % 2) throw std::logic_error{odd_num_loops_error("connectivity_base", i)};
          },
          [&](edge_index_type i, edge_index_type target, range_t hostRange, range_t targetRange) {
            const auto brokenEdge{
              [&, i, target, hostRange, targetRange]() -> std::optional<edge_indices> {
                if(auto reciprocalCount{std::ranges::distance(targetRange)}; !reciprocalCount)
                {
                  return edge_indices{i, static_cast<std::size_t>(std::ranges::distance(edges.cbegin_partition(i), hostRange.begin()))};
                }
                else if(auto count{std::ranges::distance(hostRange)}; count > reciprocalCount)
                {
                  return edge_indices{i, static_cast<std::size_t>(std::ranges::distance(edges.cbegin_partition(i), hostRange.begin()) + reciprocalCount)};
                }
                else if(count < reciprocalCount)
                {
                  return edge_indices{target, static_cast<std::size_t>(std::ranges::distance(edges.cbegin_partition(target), targetRange.begin()) + count)};
                }

                return {};
              }()
            };

            if(brokenEdge)
              throw std::logic_error{absent_reciprocated_partial_edge_message("connectivity_base", brokenEdge.value())};
          }
        );

        return std::forward<IntermediateEdges>(edges);
      }

      template<std::ranges::view View>
      constexpr static auto find_cluster_end(View v) -> decltype(v.begin())
      {
        return !v.empty() ? std::ranges::find_if_not(v, [&front{v.front()}](const auto& e){ return e == front; }) : v.end();
      }

      template<class Edges, alloc... Allocators>
      [[nodiscard]]
      constexpr edge_storage_type process_edges(Edges&& orderedEdges, const Allocators&... as)
        requires direct_init_v
      {
        return {std::forward<Edges>(orderedEdges), as...};
      }

      template<alloc... Allocators>
      [[nodiscard]]
      constexpr edge_storage_type process_edges(edges_initializer edges, const Allocators&... as)
        requires (!direct_init_v && !is_embedded(flavour))
      {
        edge_storage_type storage(as...);
        storage.reserve_partitions(edges.size());

        for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
        {
          storage.add_slot();

          const auto nodeIndex{static_cast<edge_index_type>(std::ranges::distance(edges.begin(), nodeEdgesIter))};
          const auto& nodeEdges{*nodeEdgesIter};
          for(const auto& edge : nodeEdges)
          {
            storage.push_back_to_partition(nodeIndex, edge.target_node(), edge.weight());
          }
        }

        return storage;
      }

      template<alloc... Allocators>
      [[nodiscard]]
      constexpr edge_storage_type process_edges(edges_initializer edges, const Allocators&... as)
        requires (!direct_init_v && is_embedded(flavour))
      {
        edge_storage_type storage(as...);
        storage.reserve_partitions(edges.size());

        for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
        {
          storage.add_slot();

          const auto nodeIndex{static_cast<edge_index_type>(std::ranges::distance(edges.begin(), nodeEdgesIter))};
          const auto& nodeEdges{*nodeEdgesIter};
          for(const edge_init_type& edge : nodeEdges)
          {
            storage.push_back_to_partition(nodeIndex, edge_type{edge});
          }
        }

        return storage;
      }

      template<class Edges, alloc... Allocators>
      [[nodiscard]]
      constexpr edge_storage_type process_edges(const Edges& orderedEdges, const Allocators&... as)
        requires (!direct_init_v && !is_embedded(flavour) && !is_directed(flavour))
      {
        using range_t = partition_iterator_range<Edges>;

        edge_storage_type storage(as...);
        storage.reserve_partitions(orderedEdges.num_partitions());

        auto addToStorage{
          [&storage,this](edge_index_type host, edge_index_type target, edge_index_type compIndex, range_t hostRange){
              storage.push_back_to_partition(host, (compIndex == npos) ? edge_type{hostRange.front()}
                                                                       : edge_type{target, *(cbegin_edges(target) + compIndex)});
          }
        };

        visit_edges(
          orderedEdges,
          [&storage]() { storage.add_slot(); },
          [addToStorage,&orderedEdges](edge_index_type host, range_t hostRange) {
            for(; hostRange.begin() != hostRange.end(); hostRange.advance(1))
            {
              const bool hasCompIndex{(std::ranges::distance(hostRange) % 2) && shared_weight_v};
              const auto compIndex{hasCompIndex ? static_cast<edge_index_type>(std::ranges::distance(orderedEdges.cbegin_partition(host), hostRange.begin()) - 1) : npos};

              addToStorage(host, host, compIndex, hostRange);
            }
          },
          [addToStorage,&orderedEdges](edge_index_type host, edge_index_type target, range_t hostRange, range_t targetRange) {
            for(; hostRange.begin() != hostRange.end(); hostRange.advance(1))
            {
              const bool hasCompIndex{(host > target) && shared_weight_v};
              const auto compIndex{hasCompIndex ? static_cast<edge_index_type>(std::ranges::distance(orderedEdges.cbegin_partition(target), targetRange.end()) - std::ranges::distance(hostRange)) : npos};

              addToStorage(host, target, compIndex, hostRange);
            }
          }
        );

        return storage;
      }

      template<class Edges,
               std::invocable PerNodeFn,
               std::invocable<edge_index_type, partition_iterator_range<Edges>> PerLoopFn,
               std::invocable<edge_index_type, edge_index_type, partition_iterator_range<Edges>, partition_iterator_range<Edges>> PerLinkFn
      >
      constexpr static void visit_edges(const Edges& orderedEdges, PerNodeFn perNode, PerLoopFn perLoop, PerLinkFn perLink)
        requires (!is_embedded(flavour) && !is_directed(flavour))
      {
        constexpr bool clusterEdges{!std::is_empty_v<edge_weight_type> && !std::totally_ordered<edge_weight_type>};

        for(edge_index_type i{}; i < orderedEdges.num_partitions(); ++i)
        {
          perNode();

          auto lowerIter{orderedEdges.cbegin_partition(i)}, upperIter{orderedEdges.cbegin_partition(i)};
          while(lowerIter != orderedEdges.cend_partition(i))
          {
            upperIter = std::ranges::upper_bound(lowerIter, orderedEdges.cend_partition(i), *lowerIter, graph_impl::edge_comparer{});
            if constexpr(clusterEdges) upperIter = find_cluster_end(std::ranges::subrange{lowerIter, upperIter});

            if(const auto target{lowerIter->target_node()}; target == i)
            {
              perLoop(i, {lowerIter, upperIter});
            }
            else
            {
              graph_errors::check_edge_index_range("", {i, static_cast<std::size_t>(std::ranges::distance(orderedEdges.cbegin_partition(i), lowerIter))}, "target", orderedEdges.num_partitions(), lowerIter->target_node());

              const auto comparisonEdge{
                [lowerIter, i]() {
                  auto compEdge{*lowerIter};
                  compEdge.target_node(i);
                  return compEdge;
                }()
              };

              auto eqrange{std::ranges::equal_range(orderedEdges.partition(target), comparisonEdge, graph_impl::edge_comparer{})};

              if constexpr(clusterEdges)
              {
                auto clusters{std::views::drop_while(eqrange, [&wt{lowerIter->weight()}](const auto& e) { return e.weight() != wt; })};
                eqrange = {clusters.begin(), find_cluster_end(clusters)};
              }

              perLink(i, target, {lowerIter, upperIter}, eqrange);
            }

            lowerIter = upperIter;
          }
        }
      }

      template<alloc... Allocators>
      [[nodiscard]]
      edge_storage_type copy_edges(const connectivity_base& in, const Allocators&... as)
        requires direct_copy_v
      {
        return edge_storage_type{in.m_Edges, as...};
      }

      [[nodiscard]]
      edge_storage_type copy_edges(const connectivity_base& in)
        requires (!direct_copy_v)
      {
        if constexpr(has_partitions_allocator<edge_storage_type>)
        {
          return copy_edges(in, in.m_Edges.get_allocator(), in.m_Edges.get_partitions_allocator());
        }
        else
        {
          return copy_edges(in, in.m_Edges.get_allocator());
        }
      }

      template<alloc... Allocators>
      [[nodiscard]]
      edge_storage_type copy_edges(const connectivity_base& in, const Allocators&... as)
        requires (!direct_copy_v && (sizeof...(Allocators) > 0))
      {        
        edge_storage_type storage({std::allocator_traits<Allocators>::select_on_container_copy_construction(as)}...);

        if constexpr(edge_type::flavour == edge_flavour::partial)
        {
          copy_edges(in, storage, graph_impl::edge_maker<edge_type, edge_storage_type>{storage});
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          auto processor{
            [this, &storage](const size_type node, const_edge_iterator first, const_edge_iterator i) {
              const auto dist{static_cast<edge_index_type>(std::ranges::distance(first, i))};
              const bool encountered{(i->target_node() < node)
                  || ((i->target_node() == node) && (i->complementary_index() < dist))};

              if(!encountered)
              {
                storage.push_back_to_partition(node, i->target_node(), i->complementary_index(), i->weight());
              }
              else
              {
                const auto compI{i->complementary_index()};
                storage.push_back_to_partition(node, i->target_node(), compI, *(cbegin_edges(i->target_node()) + compI));
              }
            }
          };

          copy_edges(in, storage, processor);
        }

        return storage;
      }

      template<std::invocable<size_type, const_edge_iterator, const_edge_iterator> Processor>
      void copy_edges(const connectivity_base& in, edge_storage_type& storage, Processor processor)
      {
        reserve_nodes(in.m_Edges.num_partitions());
        if constexpr(!graph_impl::has_reservable_partitions<edge_storage_type>)
        {
          storage.reserve(in.m_Edges.size());
        }
        for(size_type i{}; i<in.order(); ++i)
        {
          storage.add_slot();
          if constexpr(graph_impl::has_reservable_partitions<edge_storage_type>)
          {
            storage.reserve_partition(i, std::ranges::distance(in.cbegin_edges(i), in.cend_edges(i)));
          }
          for(auto inIter{in.cbegin_edges(i)}; inIter != in.cend_edges(i); ++inIter)
          {
            processor(i, in.cbegin_edges(i), inIter);
          }
        }
      }

      [[nodiscard]]
      constexpr const_edge_iterator to_const_edge_iterator(const_reverse_edge_iterator criter) const
      {
        const auto source{criter.partition_index()};
        return m_Edges.cbegin_partition(source) + std::ranges::distance(criter, m_Edges.crend_partition(source)) - 1;
      }

      [[nodiscard]]
      constexpr edge_iterator to_edge_iterator(const_edge_iterator citer)
      {
        const auto source{citer.partition_index()};
        const auto dist{std::ranges::distance(cbegin_edges(source), citer)};
        return m_Edges.begin_partition(source) + dist;
      }

      template<std::invocable<edge_weight_type&> Fn>
      constexpr std::invoke_result_t<Fn, edge_weight_type&> mutate_source_edge_weight(const_edge_iterator citer, Fn fn)
      {
        return fn(to_edge_iterator(citer)->weight());
      }

      template<class... Args>
      constexpr void set_source_edge_weight(edge_iterator iter, Args&&... args)
      {
        iter->weight(std::forward<Args>(args)...);
      }

      template<class Setter>
        requires std::is_invocable_r_v<edge_iterator, Setter, edge_iterator>
      constexpr const_edge_iterator manipulate_partner_edge_weight(const_edge_iterator citer, Setter setter)
      {
        const auto partner{citer->target_node()};

        if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          const auto comp{citer->complementary_index()};
          return setter(m_Edges.begin_partition(partner) + comp);
        }
        else
        {
          
          auto found{end_edges(partner)};
          if(const auto source{citer.partition_index()}; source == partner)
          {
            for(auto iter{m_Edges.begin_partition(partner)}; iter != m_Edges.end_partition(partner); ++iter)
            {
              if(std::ranges::distance(m_Edges.begin_partition(partner), iter) == std::ranges::distance(cbegin_edges(partner), citer))
                continue;

              if(*iter == *citer)
              {
                found = setter(iter);
                break;
              }
            }
          }
          else
          {
            for(auto iter{m_Edges.begin_partition(partner)}; iter != m_Edges.end_partition(partner); ++iter)
            {
              if((iter->target_node() == source) && (iter->weight() == citer->weight()))
              {
                found = setter(iter);
                break;
              }
            }
          }

          if(found != end_edges(partner))
          {
            return found;
          }
          else
          {
            const auto node{citer.partition_index()};
            const auto edgeIndex{static_cast<std::size_t>(std::ranges::distance(cbegin_edges(node), citer))};
            throw std::logic_error{ graph_errors::absent_partner_weight_message("manipulate_partner_edge_weight", {node, edgeIndex}) };
          }
        }
      }

      template<std::invocable<edge_weight_type&> Fn>
      constexpr void mutate_partner_edge_weight(const_edge_iterator citer, Fn fn)
      {
        manipulate_partner_edge_weight(citer, [fn](edge_iterator iter) -> edge_iterator { fn(iter->weight()); return iter; });
      }

      template<class... Args>
        requires initializable_from<edge_weight_type, Args...>
      constexpr const_edge_iterator set_partner_edge_weight(const_edge_iterator citer, Args&&... args)
      {
        return manipulate_partner_edge_weight(citer, [&args...](edge_iterator iter) -> edge_iterator { iter->weight(std::forward<Args>(args)...); return iter; });
      }

      template<class... MetaData>
        requires std::is_copy_constructible_v<edge_type> && (std::is_same_v<MetaData, edge_meta_data_type> && ...)
      void reciprocal_join(const edge_index_type node1, const edge_index_type node2, MetaData... md)
      {
        graph_impl::join_sentinel sentinel{m_Edges, node1, m_Edges.size_of_partition(node1) - 1};
        if constexpr(edge_type::flavour == edge_flavour::partial)
        {
          m_Edges.push_back_to_partition(node2, node1, std::move(md)..., *crbegin_edges(node1));
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          const edge_index_type compIndex(std::ranges::distance(cbegin_edges(node1), cend_edges(node1)) - 1);
          m_Edges.push_back_to_partition(node2, node1, compIndex, std::move(md)..., *crbegin_edges(node1));
        }
      }


      template<class... MetaData>
        requires (is_embedded(flavour) && std::is_copy_constructible_v<edge_type> && (std::is_same_v<edge_meta_data_type, MetaData> && ...))
      std::pair<const_edge_iterator, const_edge_iterator>
        insert_reciprocal_join(const_edge_iterator citer1, const edge_index_type pos2, MetaData... args)
      {
        const auto node{citer1.partition_index()};

        auto pos1{static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(node), citer1))};
        if(pos2 <= pos1) ++pos1;

        const auto dist1{static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(node), citer1))};
        graph_impl::join_sentinel sentinel{m_Edges, node, dist1};

        auto citer2{m_Edges.insert_to_partition(cbegin_edges(node) + pos2, node, pos1, std::move(args)..., *citer1)};
        if(pos2 > pos1)
        {
          increment_comp_indices(++to_edge_iterator(citer2), end_edges(node), 1);
        }
        else
        {
          auto iter1{begin_edges(node) + pos1};
          increment_comp_indices(++to_edge_iterator(citer2), iter1, 1);
          increment_comp_indices(iter1 + 1, end_edges(node), 1);
        }

        citer1 = cbegin_edges(node) + pos1;
        return {citer1, citer2};
      }

      template<class... MetaData>
        requires (is_embedded(flavour) && std::is_copy_constructible_v<edge_type> && (std::is_same_v<edge_meta_data_type, MetaData> && ...))
      std::pair<const_edge_iterator, const_edge_iterator>
        insert_reciprocal_join(const_edge_iterator citer1, const_edge_iterator citer2, MetaData... md)
      {
        const auto node1{citer1.partition_index()}, node2{citer2.partition_index()};
        const auto dist1{static_cast<edge_index_type>(std::ranges::distance(cbegin_edges(node1), citer1))};

        graph_impl::join_sentinel sentinel{m_Edges, node1, dist1};
        citer2 = m_Edges.insert_to_partition(citer2, node1, dist1, md..., *citer1);
        increment_comp_indices(++to_edge_iterator(citer2), end_edges(node2), 1);

        citer1 = cbegin_edges(node1) + dist1;
        return {citer1, citer2};
      }

      template<class... Args>
      void add_to_partition(const edge_index_type node1, const edge_index_type node2, Args&&... args)
      {
        if constexpr (edge_type::flavour == edge_flavour::partial)
        {
          m_Edges.push_back_to_partition(node1, node2, std::forward<Args>(args)...);
        }
        else if constexpr (edge_type::flavour == edge_flavour::partial_embedded)
        {
          const edge_index_type dist(std::ranges::distance(cbegin_edges(node2), cend_edges(node2)));
          const edge_index_type i{ node1 == node2 ? dist + 1 : dist };
          m_Edges.push_back_to_partition(node1, node2, i, std::forward<Args>(args)...);
        }
      }

      template<class... Args>
      const_edge_iterator insert_to_partition(const_edge_iterator citer1, const edge_index_type node2, const edge_index_type pos2, Args&&... args)
      {
        const auto node1{ citer1.partition_index() };
        citer1 = m_Edges.insert_to_partition(citer1, node2, pos2, std::forward<Args>(args)...);
        increment_comp_indices(++to_edge_iterator(citer1), end_edges(node1), 1);

        return citer1;
      }

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel, class Fn>
        requires std::is_invocable_r_v<edge_index_type, Fn, edge_index_type>
      constexpr void modify_comp_indices(Iter first, Sentinel last, Fn fn) noexcept
      {
        while(first != last)
        {
          const auto source{first.partition_index()};
          const auto other{first->target_node()};

          auto setPartnerCompIndex{
            [this, other, fn](edge_index_type cmpIndex){
              const auto edgeIter{m_Edges.begin_partition(other) + cmpIndex};
              edgeIter->complementary_index(fn(edgeIter->complementary_index()));
            }
          };

          const auto compIndex{first->complementary_index()};
          if(source != other)
          {
            setPartnerCompIndex(compIndex);
          }
          else
          {
            const auto dist{static_cast<edge_index_type>(std::ranges::distance(begin_edges(source), first))};
            if(const auto shiftedCompIndex{fn(compIndex)}; shiftedCompIndex <= dist)
            {
              setPartnerCompIndex(compIndex);
            }
            else
            {
              setPartnerCompIndex(shiftedCompIndex);
            }
          }

          ++first;
        }
      }

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      constexpr void decrement_comp_indices(Iter first, Sentinel last, const edge_index_type num) noexcept
      {
        modify_comp_indices(first, last, [num](const auto compIndex) { return compIndex >= num ? compIndex - num : edge_index_type{}; });
      }

      template<std::input_or_output_iterator Iter, std::sentinel_for<Iter> Sentinel>
      constexpr void increment_comp_indices(Iter first, Sentinel last, const edge_index_type num) noexcept
      {
        modify_comp_indices(first, last, [num](const auto compIndex) { return compIndex + num; });
      }

      template<class Pred, class Modifier>
      constexpr void fix_edge_data(const edge_index_type node, Pred pred, Modifier modifier) noexcept
      {
        for(size_type i{}; i < m_Edges.num_partitions(); ++i)
        {
          for(auto& edge : m_Edges.partition(i))
          {
            const auto target{edge.target_node()};
            if(pred(target, node)) edge.target_node(modifier(target));
          }
        }
      }
    };

    template<std::input_or_output_iterator Iterator>
    class edge_weight_dereference_policy
    {
    public:
      constexpr static bool is_const{is_const_reference_v<typename std::iterator_traits<Iterator>::reference>};

      using value_type = typename std::iterator_traits<Iterator>::value_type::weight_type;
      using reference  = std::conditional_t<is_const, const value_type&, value_type&>;

      constexpr edge_weight_dereference_policy() = default;
      constexpr edge_weight_dereference_policy(const edge_weight_dereference_policy&) = default;

      [[nodiscard]]
      friend constexpr bool operator==(const edge_weight_dereference_policy&, const edge_weight_dereference_policy&) noexcept = default;

      [[nodiscard]]
      static reference get(Iterator i)
      {
        return i->weight();
      }
    protected:
      constexpr edge_weight_dereference_policy(edge_weight_dereference_policy&&) noexcept = default;

      ~edge_weight_dereference_policy() = default;

      constexpr edge_weight_dereference_policy& operator=(const edge_weight_dereference_policy&) = default;
      constexpr edge_weight_dereference_policy& operator=(edge_weight_dereference_policy&&) noexcept = default;
    };

    template<graph_flavour Flavour, class EdgeStorage>
    class connectivity : public connectivity_base<Flavour, EdgeStorage>
    {
    protected:
      using connectivity_base<Flavour, EdgeStorage>::connectivity_base;
    };

    template<class EdgeStorage>
      requires (!std::is_empty_v<typename EdgeStorage::value_type::weight_type>)
    class connectivity<graph_flavour::directed, EdgeStorage> : public connectivity_base<graph_flavour::directed, EdgeStorage>
    {
    public:
      using base_type = connectivity_base<graph_flavour::directed, EdgeStorage>;

      using edge_weight_iterator               = utilities::iterator<typename base_type::edge_iterator,               edge_weight_dereference_policy<typename base_type::edge_iterator>>;
      using const_edge_weight_iterator         = utilities::iterator<typename base_type::const_edge_iterator,         edge_weight_dereference_policy<typename base_type::const_edge_iterator>>;
      using reverse_edge_weight_iterator       = utilities::iterator<typename base_type::reverse_edge_iterator,       edge_weight_dereference_policy<typename base_type::reverse_edge_iterator>>;
      using const_reverse_edge_weight_iterator = utilities::iterator<typename base_type::const_reverse_edge_iterator, edge_weight_dereference_policy<typename base_type::const_reverse_edge_iterator>>;
      using edge_weights_range                 = std::ranges::subrange<edge_weight_iterator>;
      using const_edge_weights_range           = std::ranges::subrange<const_edge_weight_iterator>;

      using edge_index_type = typename base_type::edge_index_type;

      [[nodiscard]]
      constexpr edge_weight_iterator begin_edge_weights(const edge_index_type node)
      {
        return edge_weight_iterator{this->begin_edges(node)};
      }

      [[nodiscard]]
      constexpr edge_weight_iterator end_edge_weights(const edge_index_type node)
      {
        return edge_weight_iterator{this->end_edges(node)};
      }

      [[nodiscard]]
      constexpr const_edge_weight_iterator begin_edge_weights(const edge_index_type node) const
      {
        return const_edge_weight_iterator{this->begin_edges(node)};
      }

      [[nodiscard]]
      constexpr const_edge_weight_iterator end_edge_weights(const edge_index_type node) const
      {
        return const_edge_weight_iterator{this->end_edges(node)};
      }

      [[nodiscard]]
      constexpr const_edge_weight_iterator cbegin_edge_weights(const edge_index_type node) const
      {
        return const_edge_weight_iterator{this->begin_edges(node)};
      }

      [[nodiscard]]
      constexpr const_edge_weight_iterator cend_edge_weights(const edge_index_type node) const
      {
        return const_edge_weight_iterator{this->end_edges(node)};
      }

      [[nodiscard]]
      constexpr reverse_edge_weight_iterator rbegin_edge_weights(const edge_index_type node)
      {
        return reverse_edge_weight_iterator{this->rbegin_edges(node)};
      }

      [[nodiscard]]
      constexpr reverse_edge_weight_iterator rend_edge_weights(const edge_index_type node)
      {
        return reverse_edge_weight_iterator{this->rend_edges(node)};
      }

      [[nodiscard]]
      constexpr const_reverse_edge_weight_iterator rbegin_edge_weights(const edge_index_type node) const
      {
        return const_reverse_edge_weight_iterator{this->rbegin_edges(node)};
      }

      [[nodiscard]]
      constexpr const_reverse_edge_weight_iterator rend_edge_weights(const edge_index_type node) const
      {
        return const_reverse_edge_weight_iterator{this->rend_edges(node)};
      }

      [[nodiscard]]
      constexpr const_reverse_edge_weight_iterator crbegin_edge_weights(const edge_index_type node) const
      {
        return const_reverse_edge_weight_iterator{this->rbegin_edges(node)};
      }

      [[nodiscard]]
      constexpr const_reverse_edge_weight_iterator crend_edge_weights(const edge_index_type node) const
      {
        return const_reverse_edge_weight_iterator{this->rend_edges(node)};
      }

      [[nodiscard]]
      constexpr edge_weights_range edge_weights(const edge_index_type node) { return {begin_edge_weights(node), end_edge_weights(node)}; }

      [[nodiscard]]
      constexpr const_edge_weights_range edge_weights(const edge_index_type node) const { return {begin_edge_weights(node), end_edge_weights(node)}; }

      [[nodiscard]]
      constexpr const_edge_weights_range cedge_weights(const edge_index_type node) const { return {begin_edge_weights(node), end_edge_weights(node)}; }
    protected:
      using connectivity_base<graph_flavour::directed, EdgeStorage>::connectivity_base;
    };
  }
}
