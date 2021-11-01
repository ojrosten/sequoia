////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Implementation for the storage of graph edges.
 */

#include "sequoia/Core/DataStructures/DataStructuresTypeTraits.hpp"
#include "sequoia/Maths/Graph/GraphDetails.hpp"
#include "sequoia/Maths/Graph/GraphTraits.hpp"
#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/Core/Utilities/AssignmentUtilities.hpp"
#include "sequoia/Core/Ownership/HandlerTraits.hpp"

#include <limits>
#include <set>
#include <map>
#include <stdexcept>

namespace sequoia
{
  namespace data_structures
  {
    template <class, class H, class> requires ownership::handler<H> class bucketed_storage;
    template <class, class H>        requires ownership::handler<H> struct bucketed_storage_traits;
    template <class, class H, class> requires ownership::handler<H> class partitioned_sequence;
    template <class, class H>        requires ownership::handler<H> struct partitioned_sequence_traits;
    template <class, std::size_t, std::size_t, std::integral> class static_partitioned_sequence;
    class static_data_base;
  }

  namespace maths
  {
    struct partitions_allocator_tag{};

    template
    <
      directed_flavour Directedness,
      class EdgeTraits,
      class WeightMaker
    >
    class connectivity : private WeightMaker
    {
      friend struct sequoia::impl::assignment_helper;
    protected:
      using edge_storage_type = typename EdgeTraits::edge_storage_type;
    public:

      using edge_type                   = typename EdgeTraits::edge_type;
      using edge_weight_type            = typename edge_type::weight_type;
      using edge_index_type             = typename edge_type::index_type;
      using edge_init_type              = typename EdgeTraits::edge_init_type;
      using size_type                   = typename edge_storage_type::size_type;
      using weight_maker_type           = WeightMaker;
      using const_edge_iterator         = typename edge_storage_type::const_partition_iterator;
      using const_reverse_edge_iterator = typename edge_storage_type::const_reverse_partition_iterator;

      static_assert(std::is_unsigned_v<edge_index_type>);

      constexpr static auto npos{std::numeric_limits<edge_index_type>::max()};
      constexpr static directed_flavour directedness{Directedness};
      constexpr static bool throw_on_range_error{edge_storage_type::throw_on_range_error};

      constexpr connectivity() = default;

      constexpr connectivity(std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : connectivity(direct_edge_init(), edges)
      {}

      constexpr connectivity(const connectivity& in)
        : connectivity(direct_edge_copy(), in)
      {}

      [[nodiscard]]
      constexpr size_type order() const noexcept { return m_Edges.num_partitions(); }

      [[nodiscard]]
      constexpr size_type size() const noexcept
      {
        if constexpr (EdgeTraits::mutual_info_v)
        {
          return m_Edges.size() / 2;
        }
        else
        {
          return m_Edges.size();
        }
      }

      [[nodiscard]]
      constexpr const_edge_iterator cbegin_edges(const edge_index_type node) const
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.cbegin_partition(node);
      }

      [[nodiscard]]
      constexpr const_edge_iterator cend_edges(const edge_index_type node) const
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.cend_partition(node);
      }

      [[nodiscard]]
      constexpr const_reverse_edge_iterator crbegin_edges(const edge_index_type node) const
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.crbegin_partition(node);
      }

      [[nodiscard]]
      constexpr const_reverse_edge_iterator crend_edges(const edge_index_type node) const
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.crend_partition(node);
      }

      template<class Fn>
      constexpr void mutate_edge_weight(const_edge_iterator citer, Fn fn)
      {
        static_assert(!std::is_empty_v<edge_weight_type>, "Cannot mutate an empty weight!");

        if constexpr (!EdgeTraits::shared_weight_v && !EdgeTraits::shared_edge_v && EdgeTraits::mutual_info_v)
        {
          mutate_partner_edge_weight(citer, fn);
          mutate_source_edge_weight(citer, fn);
        }
        else
        {
          mutate_source_edge_weight(citer, fn);
        }
      }

      template<class Fn>
      constexpr void mutate_edge_weight(const_reverse_edge_iterator criter, Fn fn)
      {
        const auto source{criter.partition_index()};
        mutate_edge_weight(m_Edges.cbegin_partition(source) + distance(criter, m_Edges.crend_partition(source)) - 1, fn);
      }

      template<class Arg, class... Args>
      constexpr void set_edge_weight(const_edge_iterator citer, Arg&& arg, Args&&... args)
      {
        static_assert(!std::is_empty_v<edge_weight_type>, "Cannot set an empty weight!");

        if constexpr (!EdgeTraits::shared_weight_v && !EdgeTraits::shared_edge_v && EdgeTraits::mutual_info_v)
        {
          if constexpr(EdgeTraits::weight_setting_exception_guarantee_v)
          {
            auto partnerSetter{
              [this](const_edge_iterator citer, auto&&... args){
                return this->set_partner_edge_weight(citer, std::forward<decltype(args)>(args)...);
              }
            };

            auto sourceSetter{
              [this](const_edge_iterator citer, const_edge_iterator partnerIter){
                this->set_source_edge_weight(citer, partnerIter->weight());
              }
            };

            weight_sentinel sentinel{*this, citer, partnerSetter, sourceSetter,
                                            std::forward<Arg>(arg), std::forward<Args>(args)...};
          }
          else
          {
            auto partnerIter{set_partner_edge_weight(citer, std::forward<Arg>(arg), std::forward<Args>(args)...)};
            set_source_edge_weight(citer, partnerIter->weight());
          }
        }
        else
        {
          set_source_edge_weight(citer, std::forward<Arg>(arg), std::forward<Args>(args)...);
        }
      }

      template<class Arg, class... Args>
      constexpr void set_edge_weight(const_reverse_edge_iterator criter, Arg&& arg, Args&&... args)
      {
        const auto source{criter.partition_index()};
        set_edge_weight(m_Edges.cbegin_partition(source) + distance(criter, m_Edges.crend_partition(source)) - 1, std::forward<Arg>(arg), std::forward<Args>(args)...);
      }

      //===============================equality (not isomorphism) operators================================//

      [[nodiscard]]
      friend constexpr bool operator==(const connectivity& lhs, const connectivity& rhs) noexcept
      {
        return lhs.m_Edges == rhs.m_Edges;
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const connectivity& lhs, const connectivity& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    protected:
      using edge_iterator = typename edge_storage_type::partition_iterator;
      using init_t = std::initializer_list<std::initializer_list<edge_init_type>>;

      template<alloc... Allocators>
      constexpr connectivity(const Allocators&... as)
        : m_Edges(as...)
      {}

      template<alloc... Allocators>
      constexpr connectivity(init_t edges, const Allocators&... as)
        : connectivity(direct_edge_init(), edges, as...)
      {}

      template<alloc... Allocators>
      constexpr connectivity(const connectivity& c, const Allocators&... as)
        : connectivity(direct_edge_copy(), c, as...)
      {}

      constexpr connectivity(connectivity&&) noexcept = default;

      template<alloc... Allocators>
      constexpr connectivity(connectivity&& c, const Allocators&... as)
        : m_Edges{std::move(c.m_Edges), as...}
      {}

      ~connectivity() = default;

      constexpr connectivity& operator=(connectivity&&) = default;

      constexpr connectivity& operator=(const connectivity& in)
      {
        if(&in != this)
        {
          auto allocGetter{
            [](const connectivity& in){
              if constexpr(has_allocator_type<edge_storage_type>)
              {
                return in.m_Edges.get_allocator();
              }
            }
          };

          auto partitionsAllocGetter{
            [](const connectivity& in){
              if constexpr(has_partitions_allocator<edge_storage_type>)
              {
                return in.m_Edges.get_partitions_allocator();
              }
            }
          };

          sequoia::impl::assignment_helper::assign(*this, in, allocGetter, partitionsAllocGetter);
        }

        return *this;
      }

      void swap(connectivity& rhs)
        // TO DO: Strictly speaking incorrect but will be fine when ranges::swap available
        noexcept(noexcept(std::swap(this->m_Edges, rhs.m_Edges)))
      {
        using std::swap;
        swap(m_Edges, rhs.m_Edges);
      }

      constexpr void swap_edges(edge_index_type node, edge_index_type i, edge_index_type j)
      {
        std::iter_swap(begin_edges(node) + i, begin_edges(node) + j);
      }

      constexpr void swap_nodes(size_type i, size_type j)
      {
        if(!size())
        {
          if constexpr(throw_on_range_error)
          {
            if((i >= order()) || (j >= order()))
              throw std::out_of_range{"Connectivity::swap_nodes: indices out of range"};
          }

          return;
        }

        if constexpr (EdgeTraits::shared_edge_v)
        {
          for(auto iter{begin_edges(i)}; iter != end_edges(i); ++iter)
          {
            if((iter->source_node() != j) && (iter->target_node() != j))
            {
              if(iter->source_node() == i) iter->source_node(j);
              else if(iter->source_node() == j) iter->source_node(i);

              if(iter->target_node() == i) iter->target_node(j);
              else if(iter->target_node() == j) iter->target_node(i);
            }
          }

          for(auto iter{begin_edges(j)}; iter != end_edges(j); ++iter)
          {
            if(iter->source_node() == i) iter->source_node(j);
            else if(iter->source_node() == j) iter->source_node(i);

            if(iter->target_node() == i) iter->target_node(j);
            else if(iter->target_node() == j) iter->target_node(i);
          }
        }
        else
        {
          for(size_type n{}; n<order(); ++n)
          {

            for(auto iter{begin_edges(n)}; iter != end_edges(n); ++iter)
            {
              if constexpr (EdgeTraits::mutual_info_v && directed(directedness))
              {
                if(iter->source_node() == i) iter->source_node(j);
                else if(iter->source_node() == j) iter->source_node(i);
              }

              if(iter->target_node() == i) iter->target_node(j);
              else if(iter->target_node() == j) iter->target_node(i);
            }
          }
        }

        m_Edges.swap_partitions(i, j);
      }

      auto get_edge_allocator() const
      {
        return m_Edges.get_allocator();
      }

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

      [[nodiscard]]
      constexpr edge_iterator begin_edges(const edge_index_type node)
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.begin_partition(node);
      }

      [[nodiscard]]
      constexpr edge_iterator end_edges(const edge_index_type node)
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.end_partition(node);
      }

      void add_node()
      {
        m_Edges.add_slot();
      }

      void insert_node(const size_type node)
      {
        m_Edges.insert_slot(node);
        fix_edge_data(node,
                      [](const auto targetNode, const auto node) { return targetNode >= node; },
                      [](const auto index) { return index + 1; });
      }

      void erase_node(const size_type node)
      {
        if constexpr (throw_on_range_error) if(node >= order()) throw std::out_of_range("Cannot erase node: index out of range");

        if constexpr (EdgeTraits::mutual_info_v)
        {
          std::set<size_type> partitionsToVisit{};
          for(auto citer{m_Edges.cbegin_partition(node)}; citer != m_Edges.cend_partition(node); ++citer)
          {
            const auto target{citer->target_node()};
            if(target != node) partitionsToVisit.insert(target);

            if constexpr (directed(directedness))
            {
              const auto source{citer->source_node()};
              if(source != node) partitionsToVisit.insert(source);
            }
          }

          for(const auto partition : partitionsToVisit)
          {
            auto fn = [node](const edge_type& e) {
              if constexpr (directed(directedness))
              {
                return (e.target_node() == node) || (e.source_node() == node);
              }
              else
              {
                return e.target_node() == node;
              }
            };

            if constexpr (embeddedEdge)
            {
              for(auto iter{m_Edges.begin_partition(partition)}; iter != m_Edges.end_partition(partition);)
              {
                if(fn(*iter))
                {
                  iter = m_Edges.erase_from_partition(iter);
                  decrement_comp_indices(iter,  m_Edges.end_partition(partition), 1);
                }
                else
                {
                  ++iter;
                }
              }
            }
            else
            {
              erase_from_partition_if(m_Edges, partition, fn);
            }
          }
        }
        else
        {
          for(size_type i{}; i < m_Edges.num_partitions(); ++i)
          {
            if(i == node) continue;
            erase_from_partition_if(m_Edges, i, [node](const edge_type& e) {
                return (e.target_node() == node);
            });
          }
        }

        m_Edges.erase_slot(node);
        fix_edge_data(node,
                      [](const auto targetNode, const auto node) { return targetNode > node; },
                      [](const auto index) { return index - 1; });
      }

      void reserve_for_join([[maybe_unused]] const edge_index_type node1, [[maybe_unused]] const edge_index_type node2)
      {
        if constexpr (graph_impl::has_reservable_partitions<edge_storage_type>)
        {
          if(node1 == node2)
          {
            reserve_edges(node1, distance(cbegin_edges(node1), cend_edges(node1)) + 2);
          }
          else
          {
            reserve_edges(node1, distance(cbegin_edges(node1), cend_edges(node1)) + 1);
            reserve_edges(node2, distance(cbegin_edges(node2), cend_edges(node2)) + 2);
          }
        }
        else
        {
          reserve_edges(size() + 2);
        }
      }

      template<class... Args>
      void join(const edge_index_type node1, const edge_index_type node2, Args&&... args)
      {
        reserve_for_join(node1, node2);

        if constexpr (std::is_empty_v<edge_weight_type>)
          static_assert(sizeof...(args) == 0, "Makes no sense to supply arguments for an empty weight!");

        if constexpr (throw_on_range_error) if(node1 >= order() || node2 >= order()) throw std::out_of_range("Graph::join - index out of range");

        if constexpr(std::is_empty_v<edge_weight_type>)
        {
          if constexpr(edge_type::flavour == edge_flavour::partial)
          {
            m_Edges.push_back_to_partition(node1, node2);
          }
          else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            const edge_index_type dist(distance(cbegin_edges(node2), cend_edges(node2)));
            const edge_index_type i{node1 == node2 ? dist + 1 : dist};
            m_Edges.push_back_to_partition(node1, node2, i);
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            m_Edges.push_back_to_partition(node1, node1, node2);
          }
          else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
          {
            const edge_index_type dist(distance(cbegin_edges(node2), cend_edges(node2)));
            const edge_index_type i{node1 == node2 ? dist + 1 : dist};
            m_Edges.push_back_to_partition(node1, node1, node2, i);
          }
        }
        else
        {
          if constexpr(edge_type::flavour == edge_flavour::partial)
          {
            m_Edges.push_back_to_partition(node1, node2, make_edge_weight(std::forward<Args>(args)...));
          }
          else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            const edge_index_type dist(distance(cbegin_edges(node2), cend_edges(node2)));
            const edge_index_type i{node1 == node2 ? dist + 1 : dist};
            m_Edges.push_back_to_partition(node1, node2, i, make_edge_weight(std::forward<Args>(args)...));
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            m_Edges.push_back_to_partition(node1, node1, node2, make_edge_weight(std::forward<Args>(args)...));
          }
          else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
          {
            const edge_index_type dist(distance(cbegin_edges(node2), cend_edges(node2)));
            const edge_index_type i{node1 == node2 ? dist + 1 : dist};
            m_Edges.push_back_to_partition(node1, node1, node2, i, make_edge_weight(std::forward<Args>(args)...));
          }
        }

        if constexpr(!directed(directedness))
        {
          if constexpr(edge_type::flavour == edge_flavour::partial)
          {
            m_Edges.push_back_to_partition(node2, node1, *crbegin_edges(node1));
          }
          else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            const edge_index_type compIndex(distance(cbegin_edges(node1), cend_edges(node1)) -1);
            m_Edges.push_back_to_partition(node2, node1, compIndex, *crbegin_edges(node1));
          }
        }
        else
        {
          if constexpr(edge_type::flavour == edge_flavour::full)
          {
            m_Edges.push_back_to_partition(node2, --cend_edges(node1));
          }
          else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
          {
            const edge_index_type compIndex(distance(cbegin_edges(node1), cend_edges(node1)) -1);
            m_Edges.push_back_to_partition(node2, compIndex, *crbegin_edges(node1));
          }
        }
      }

      template<class... Args>
      std::pair<const_edge_iterator, const_edge_iterator>
      insert_join(const_edge_iterator citer1, const_edge_iterator citer2, Args&&... args)
      {
        const auto node1{citer1.partition_index()}, node2{citer2.partition_index()};
        const auto dist2{static_cast<edge_index_type>(distance(cbegin_edges(node2), citer2))};
        if(node1 == node2) return insert_join(citer1, dist2, std::forward<Args>(args)...);

        const auto dist1{static_cast<edge_index_type>(distance(cbegin_edges(node1), citer1))};
        reserve_for_join(node1, node2);

        citer1 = cbegin_edges(node1) + dist1;
        citer1 = insert_single_join(citer1, node2, dist2, std::forward<Args>(args)...);
        citer2 = cbegin_edges(node2) + dist2;

        if constexpr(EdgeTraits::shared_edge_v)
        {
          citer2 = m_Edges.insert_to_partition(citer2, citer1);
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          citer2 = m_Edges.insert_to_partition(citer2, node1, dist1, *citer1);
          increment_comp_indices(++to_edge_iterator(citer2), end_edges(node2), 1);
        }
        else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
        {
          citer2 = m_Edges.insert_to_partition(citer2, *citer1);
          increment_comp_indices(++to_edge_iterator(citer2), end_edges(node2), 1);
        }

        citer1 = cbegin_edges(node1) + dist1;
        return {citer1, citer2};
      }

      template<class... Args>
      std::pair<const_edge_iterator, const_edge_iterator>
      insert_join(const_edge_iterator citer1, const edge_index_type pos2, Args&&... args)
      {
        const auto node{citer1.partition_index()};
        const auto dist1{distance(cbegin_edges(node), citer1)};
        reserve_for_join(node, node);
        citer1 = insert_single_join(cbegin_edges(node) + dist1, node, pos2, std::forward<Args>(args)...);

        auto pos1{static_cast<edge_index_type>(distance(cbegin_edges(node), citer1))};
        if(pos2 <= pos1) ++pos1;

        if constexpr(EdgeTraits::shared_edge_v)
        {
          auto citer2{m_Edges.insert_to_partition(cbegin_edges(node) + pos2, citer1)};
          citer1 = cbegin_edges(node) + pos1;
          return {citer1, citer2};
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          auto citer2{m_Edges.insert_to_partition(cbegin_edges(node) + pos2, node, pos1, *citer1)};

          if(pos2 > pos1)
          {
            increment_comp_indices(++to_edge_iterator(citer2), end_edges(node), 1);
          }
          else
          {
            auto iter1{begin_edges(node) + pos1};
            increment_comp_indices(++to_edge_iterator(citer2), iter1, 1);
            increment_comp_indices(iter1+1, end_edges(node), 1);
          }

          citer1 = cbegin_edges(node) + pos1;
          return {citer1, citer2};
        }
      }

      void erase_edge(const_edge_iterator citer)
      {
        if constexpr (EdgeTraits::mutual_info_v)
        {
          const auto source{citer.partition_index()};
          const auto partner{partner_index(citer)};

          if(source != partner)
          {
            if constexpr (embeddedEdge)
            {
              const auto partnerLocalIndex{citer->complementary_index()};
              citer = m_Edges.erase_from_partition(citer);
              decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(source), 1);

              auto partnerIter{m_Edges.erase_from_partition(partner, partnerLocalIndex)};
              decrement_comp_indices(to_edge_iterator(partnerIter), m_Edges.end_partition(partner), 1);
            }
            else
            {
              const auto partnerDist{
                [this, partner]([[maybe_unused]] const auto source, [[maybe_unused]] const auto citer){
                  if constexpr (directed(directedness))
                  {
                    const auto pEdge{&*citer};
                    for(auto oiter = cbegin_edges(partner); oiter != cend_edges(partner); ++oiter)
                    {
                      if(&*oiter == pEdge) return distance(cbegin_edges(partner), oiter);
                    }
                  }
                  else
                  {
                    for(auto oiter{cbegin_edges(partner)}; oiter != cend_edges(partner); ++oiter)
                    {
                      if(oiter->target_node() == source)
                      {
                        if constexpr(std::is_empty_v<edge_weight_type>)
                        {
                          return distance(cbegin_edges(partner), oiter);
                        }
                        else
                        {
                          if(citer->weight() == oiter->weight())
                            return distance(cbegin_edges(partner), oiter);
                        }
                      }
                    }
                  }

                  throw std::logic_error("Deletion of undirected edge: partner not found");
                }(source, citer)};

              m_Edges.erase_from_partition(citer);
              m_Edges.erase_from_partition(cbegin_edges(partner) + partnerDist);
            }
          }
          else
          {
            if constexpr (embeddedEdge)
            {
              const auto sourceLocalIndex{citer->complementary_index()};
              const auto dist = distance(citer, cbegin_edges(source) + sourceLocalIndex);
              if(dist == 1)
              {
                citer = m_Edges.erase_from_partition(citer);
                citer = m_Edges.erase_from_partition(citer);

                decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(source), 2);
              }
              else if(dist == -1)
              {
                citer = m_Edges.erase_from_partition(citer-1);
                citer = m_Edges.erase_from_partition(citer);

                decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(source), 2);
              }
              else
              {
                const bool pred{
                  [dist, sourceLocalIndex](){
                    return (dist < 0) || (sourceLocalIndex > static_cast<edge_index_type>(dist));
                  }()
                };

                const auto eraseFirstIndex{pred ? sourceLocalIndex : dist};
                const auto eraseSecondIndex{pred ? dist : sourceLocalIndex};

                auto firstIter{m_Edges.begin_partition(source) + eraseFirstIndex};
                firstIter = m_Edges.erase_from_partition(firstIter);

                auto secondIter{m_Edges.begin_partition(source) + eraseSecondIndex};
                secondIter = m_Edges.erase_from_partition(secondIter);
                firstIter = m_Edges.begin_partition(source) + eraseFirstIndex;

                decrement_comp_indices(firstIter, m_Edges.end_partition(source), 2);
                decrement_comp_indices(secondIter, firstIter, 1);
              }
            }
            else if constexpr (EdgeTraits::shared_edge_v)
            {
              auto pEdge{&*citer};
              auto hiter{cbegin_edges(source)};
              while(hiter != cend_edges(source))
              {
                if((hiter != citer) && (&*hiter == pEdge)) break;

                ++hiter;
              }

              if(distance(hiter, citer) > 0)
              {
                m_Edges.erase_from_partition(citer);
                m_Edges.erase_from_partition(hiter);
              }
              else
              {
                m_Edges.erase_from_partition(hiter);
                m_Edges.erase_from_partition(citer);
              }
            }
            else
            {
              auto dist{distance(cbegin_edges(source), citer)};
              for(auto ci{cbegin_edges(source)}; ci != cend_edges(source); ++ci)
              {
                if((ci != citer) && (ci->target_node() == source))
                {
                  const bool remove{[]([[maybe_unused]] const auto ci, [[maybe_unused]] const auto citer){
                      if constexpr(std::is_empty_v<edge_weight_type>)
                      {
                        return true;
                      }
                      else
                      {
                        return ci->weight() == citer->weight();
                      }
                    }(ci, citer)
                  };

                  if(remove)
                  {
                    if(distance(cbegin_edges(source), ci) < dist) --dist;

                    m_Edges.erase_from_partition(ci);
                    break;
                  }
                }
              }

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

      template<class Comparer>
      constexpr void sort_edges(const_edge_iterator begin, const_edge_iterator end, Comparer comp)
      {
        static_assert(edge_type::flavour == edge_flavour::partial, "Should not attempt to sort embedded graphs");
        edge_index_type bsource{begin.partition_index()}, esource{end.partition_index()};
        if(bsource == esource)
        {
          sequoia::sort(begin_edges(bsource) + distance(cbegin_edges(bsource), begin), begin_edges(esource) + distance(cbegin_edges(esource), end), comp);
        }
        else
        {
          if(bsource > esource) std::swap(bsource, esource);
          sequoia::sort(begin_edges(bsource) + distance(cbegin_edges(bsource), begin), end_edges(bsource), comp);
          for(edge_index_type i = bsource + 1; i < esource; ++i)
          {
            sequoia::sort(begin_edges(i), end_edges(i), comp);
          }
          sequoia::sort(begin_edges(esource), begin_edges(esource) + distance(cbegin_edges(esource), end), comp);
        }
      }

    private:
      template<bool Direct>
      struct edge_init_constant : std::bool_constant<Direct>
      {};

      using direct_edge_init_type   = edge_init_constant<true>;
      using indirect_edge_init_type = edge_init_constant<false>;

      [[nodiscard]]
      static constexpr auto direct_edge_init() noexcept
      {
        if constexpr(std::is_same_v<edge_type, edge_init_type>)
        {
          return direct_edge_init_type{};
        }
        else
        {
          return indirect_edge_init_type{};
        }
      }

      static constexpr bool direct_edge_init_v{std::is_same_v<decltype(direct_edge_init()), direct_edge_init_type>};

      template<bool Direct>
      struct edge_copy_constant : std::bool_constant<Direct>
      {};

      using direct_edge_copy_type   = edge_copy_constant<true>;
      using indirect_edge_copy_type = edge_copy_constant<false>;

      [[nodiscard]]
      static constexpr auto direct_edge_copy() noexcept
      {
        return edge_copy_constant<direct_edge_init_v || (EdgeTraits::shared_edge_v && protectiveEdgeWeightProxy)>{};
      }

      template<bool HasAlloc>
      struct has_partitions_alloc_constant : std::bool_constant<HasAlloc> {};

      using has_partitions_alloc_type = has_partitions_alloc_constant<true>;
      using no_partitions_alloc_type = has_partitions_alloc_constant<false>;

      static constexpr bool protectiveEdgeWeightProxy{
        std::is_same_v<typename edge_type::weight_proxy_type, utilities::uniform_wrapper<edge_weight_type>>
      };

      static constexpr bool embeddedEdge{
        (edge_type::flavour == edge_flavour::partial_embedded) || (edge_type::flavour == edge_flavour::full_embedded)
      };

      class [[nodiscard]] weight_sentinel
      {
      public:
        template<class Fn1, class Fn2, class... Args1>
        constexpr weight_sentinel(connectivity& c, const_edge_iterator citer, Fn1 fn1, Fn2 fn2, Args1&&... args1)
          : m_Connectivity{c}
          , m_citer{citer}
          , m_OldEdgeWeight{citer->weight()}
        {
          auto partnerIter{fn1(citer, std::forward<Args1>(args1)...)};
          m_PartiallyComplete = true;

          fn2(citer, partnerIter);
          m_PartiallyComplete = false;
        }

        // Hopefully can be restored in C++20; requires p0784:
        // constexpr
        ~weight_sentinel()
        {
          if(m_PartiallyComplete)
          {
            m_Connectivity.to_edge_iterator(m_citer)->weight(std::move(m_OldEdgeWeight));
          }
        }
      private:
        connectivity& m_Connectivity;
        bool m_PartiallyComplete{};
        const_edge_iterator m_citer;
        edge_weight_type m_OldEdgeWeight;
      };

      // private data
      edge_storage_type m_Edges;

      // constructor implementations
      template<alloc... Allocators>
      constexpr connectivity(direct_edge_init_type, init_t edges, const Allocators&... as)
        : m_Edges{edges, as...}
      {
        process_edges(edges);
      }

      constexpr connectivity(indirect_edge_init_type, init_t edges)
        : m_Edges{}
      {
         process_edges(edges);
      }

      template<alloc EdgeAllocator, alloc PartitionAllocator>
        requires std::constructible_from<edge_storage_type, EdgeAllocator, PartitionAllocator>
      constexpr connectivity(indirect_edge_init_type, init_t edges, const EdgeAllocator& edgeAllocator, const PartitionAllocator& partitionAllocator)
        : m_Edges(edgeAllocator, partitionAllocator)
      {
         process_edges(edges);
      }

      template<alloc EdgeAllocator>
      constexpr connectivity(indirect_edge_init_type, init_t edges, const EdgeAllocator& edgeAllocator)
        : m_Edges(edgeAllocator)
      {
         process_edges(edges);
      }

      template<alloc... Allocators>
      constexpr connectivity(direct_edge_copy_type, const connectivity& in, const Allocators&... as)
        : m_Edges{in.m_Edges, as...}
      {}

      constexpr connectivity(indirect_edge_copy_type, const connectivity& in)
        : connectivity(has_partitions_alloc_constant<has_partitions_allocator<edge_storage_type>>{}, in)
      {}

      constexpr connectivity(has_partitions_alloc_type, const connectivity& in)
        : m_Edges(std::allocator_traits<decltype(m_Edges.get_allocator())>::select_on_container_copy_construction(in.m_Edges.get_allocator()),
                  std::allocator_traits<decltype(m_Edges.get_partitions_allocator())>::select_on_container_copy_construction(in.m_Edges.get_partitions_allocator()))
      {
        copy_edges(in);
      }

      constexpr connectivity(no_partitions_alloc_type, const connectivity& in)
        : m_Edges(std::allocator_traits<decltype(m_Edges.get_allocator())>::select_on_container_copy_construction(in.m_Edges.get_allocator()))
      {
        copy_edges(in);
      }

      template<alloc EdgeAllocator, alloc PartitionAllocator>
        requires std::constructible_from<edge_storage_type, EdgeAllocator, PartitionAllocator>
      constexpr connectivity(indirect_edge_copy_type, const connectivity& in, const EdgeAllocator& edgeAllocator, const PartitionAllocator& partitionAllocator)
        : m_Edges(std::allocator_traits<EdgeAllocator>::select_on_container_copy_construction(edgeAllocator),
                  std::allocator_traits<PartitionAllocator>::select_on_container_copy_construction(partitionAllocator))
      {
         copy_edges(in);
      }

      template<alloc EdgeAllocator>
        requires std::constructible_from<edge_storage_type, EdgeAllocator>
      constexpr connectivity(indirect_edge_copy_type, const connectivity& in, const EdgeAllocator& edgeAllocator)
        : m_Edges(std::allocator_traits<EdgeAllocator>::select_on_container_copy_construction(edgeAllocator))
      {
         copy_edges(in);
      }

      // helper methods
      template<class... Args>
      [[nodiscard]]
      decltype(auto) make_edge_weight(Args&&... args)
      {
        return WeightMaker::make(std::forward<Args>(args)...);
      }

      [[nodiscard]]
      static constexpr auto partner_index(const_edge_iterator citer)
      {
        const auto& edge{*citer};
        const auto target{edge.target_node()};
        if constexpr (directed(directedness))
        {
          const auto source{citer.partition_index()};
          return target == source ? edge.source_node() : target;
        }
        else
        {
          return target;
        }
      }

      constexpr void process_edges(std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
        if constexpr (EdgeTraits::is_embedded_v)
        {
          process_complementary_edges(edges);
        }
        else if constexpr (directed(directedness))
        {
          for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
          {
            if constexpr(!direct_edge_init()) m_Edges.add_slot();

            const auto& nodeEdges{*nodeEdgesIter};
            for(const auto& edge : nodeEdges)
            {
              const auto target{edge.target_node()};
              if(target >= edges.size()) throw std::logic_error("Target index out of range");

              if constexpr(!direct_edge_init())
              {
                const auto source{std::distance(edges.begin(), nodeEdgesIter)};
                m_Edges.push_back_to_partition(source, edge_type{edge.target_node(), make_edge_weight(edge.weight())});
              }
            }
          }
        }
        else
        {
          process_edges(direct_edge_init(), edges);
        }
      }

      constexpr void process_complementary_edges(std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
        if constexpr(!direct_edge_init()) m_Edges.reserve_partitions(edges.size());

        for(auto nodeEdgesIter{edges.begin()}; nodeEdgesIter != edges.end(); ++nodeEdgesIter)
        {
          if constexpr(!direct_edge_init()) m_Edges.add_slot();

          const auto& nodeEdges{*nodeEdgesIter};
          const auto currentNodeIndex{static_cast<edge_index_type>(std::distance(edges.begin(), nodeEdgesIter))};
          for(auto edgeIter{nodeEdges.begin()}; edgeIter != nodeEdges.end(); ++edgeIter)
          {
            const auto& edge{*edgeIter};
            const auto target{edge.target_node()};
            if(target >= edges.size()) throw std::logic_error("Target index out of range");

            const auto compIndex{edge.complementary_index()};

            bool doProcess{true};
            if constexpr(directed(directedness))
            {
              doProcess = (edge.target_node() != currentNodeIndex) || (edge.source_node() == edge.target_node());
            }

            if(doProcess)
            {
              auto targetEdgesIter = edges.begin() + target;
              if(compIndex >= targetEdgesIter->size())
                throw std::logic_error("Complementary index out of range");

              if(const auto dist{static_cast<edge_index_type>(std::distance(nodeEdges.begin(), edgeIter))};
                 (target == currentNodeIndex) && (compIndex == dist))
                throw std::logic_error("Complementary index is self-referential");

              const auto& targetEdge = *(targetEdgesIter->begin() + compIndex);
              if constexpr(!directed(directedness))
              {
                if(targetEdge.target_node() != currentNodeIndex)
                  throw std::logic_error("Reciprocated target index does not match for undirected graph");
              }
              else
              {
                if(target != targetEdge.target_node())
                  throw std::logic_error("Reciprocated target index does not match for embedded directed graph");

                if(edge.source_node() != targetEdge.source_node())
                  throw std::logic_error("Reciprocated source index does not match for embedded directed graph");
              }

              if(const auto dist{static_cast<edge_index_type>(std::distance(nodeEdges.begin(), edgeIter))};
                 targetEdge.complementary_index() != dist)
                throw std::logic_error("Reciprocated complementary index does not match" );

              if constexpr (!std::is_empty_v<edge_weight_type>)
              {
                if(edge.weight() != targetEdge.weight())
                  throw std::logic_error("Mismath between weights");
              }
            }

            if constexpr(directed(directedness))
            {
              const auto source{edge.source_node()};
              if(source >= edges.size()) throw std::logic_error("Host index out of range");
              if((source != currentNodeIndex) && (target != currentNodeIndex)) throw std::logic_error("At least one of source and target must match current node");

              if((edge.target_node() == currentNodeIndex) && (edge.source_node() != edge.target_node()))
              {
                auto sourceEdgesIter = edges.begin() + source;
                if(compIndex >= sourceEdgesIter->size()) throw std::logic_error("Complementary index out of range");

                const auto& sourceEdge = *(sourceEdgesIter->begin() + compIndex);
                if(sourceEdge.target_node() != currentNodeIndex)
                  throw std::logic_error("Reciprocated target index does not match for directed graph");
              }
            }

            if constexpr(!direct_edge_init())
            {
              if constexpr(!EdgeTraits::shared_edge_v)
              {
                m_Edges.push_back_to_partition(currentNodeIndex, make_edge(currentNodeIndex, edge));
              }
              else
              {
                const auto pos{static_cast<edge_index_type>(std::distance(nodeEdges.begin(), edgeIter))};
                const auto source{edge.source_node()};
                const auto partner{target == currentNodeIndex ? source : target};
                if((partner > currentNodeIndex) || ((partner == currentNodeIndex) && (compIndex > pos)))
                {
                  m_Edges.push_back_to_partition(currentNodeIndex, make_edge(source, edge));
                }
                else
                {
                  m_Edges.push_back_to_partition(currentNodeIndex, cbegin_edges(partner) + compIndex);
                }
              }
            }
          }
        }
      }

      constexpr void process_edges(indirect_edge_init_type, init_t edges)
      {
        using namespace data_structures;
        using traits_t = partitioned_sequence_traits<edge_init_type, ownership::independent<edge_init_type>>;
        partitioned_sequence<edge_init_type, ownership::independent<edge_init_type>, traits_t> edgesForChecking{edges};
        process_edges(edgesForChecking);
      }

      constexpr void process_edges(direct_edge_init_type, init_t)
      {
        process_edges(m_Edges);
      }

      template<class FwdIter> constexpr static FwdIter find_cluster_end(FwdIter begin, FwdIter end)
      {
        if(distance(begin, end) > 1)
        {
          auto testIter{end - 1};
          while(*testIter != *begin) --testIter;

          end = testIter + 1;
        }

        return end;
      }

      template<class Edges, alloc... Allocators>
      constexpr void process_edges(Edges& orderedEdges, const Allocators&... allocs)
      {
        constexpr bool sortWeights{!std::is_empty_v<edge_weight_type> && std::totally_ordered<edge_weight_type>};
        constexpr bool clusterEdges{!std::is_empty_v<edge_weight_type> && !std::totally_ordered<edge_weight_type>};

        auto edgeComparer{
          [](const auto& e1, const auto& e2){
            if constexpr (!sortWeights)
            {
              return e1.target_node() < e2.target_node();
            }
            else
            {
              return (e1.target_node() == e2.target_node()) ? e1.weight() < e2.weight() : e1.target_node() < e2.target_node();
            }
          }
        };

        for(edge_index_type i{}; i< orderedEdges.num_partitions(); ++i)
        {
          sequoia::sort(orderedEdges.begin_partition(i), orderedEdges.end_partition(i), edgeComparer);

          if constexpr(clusterEdges)
          {
            auto lowerIter{orderedEdges.begin_partition(i)}, upperIter{orderedEdges.begin_partition(i)};
            while(lowerIter != orderedEdges.end_partition(i))
            {
              while((upperIter != orderedEdges.end_partition(i)) && (lowerIter->target_node() == upperIter->target_node()))
                ++upperIter;

              sequoia::cluster(lowerIter, upperIter, [](const auto& e1, const auto& e2){
                return e1.weight() == e2.weight();
              });

              lowerIter = upperIter;
            }
          }
        }


        for(edge_index_type i{}; i<orderedEdges.num_partitions(); ++i)
        {
          if constexpr(!direct_edge_init_v) m_Edges.add_slot(allocs...);

          auto lowerIter{orderedEdges.cbegin_partition(i)}, upperIter{orderedEdges.cbegin_partition(i)};
          while(lowerIter != orderedEdges.cend_partition(i))
          {
            const auto target{lowerIter->target_node()};
            if(target >= orderedEdges.num_partitions())
              throw std::logic_error("Target index out of range");

            upperIter = std::upper_bound(lowerIter, orderedEdges.cend_partition(i), *lowerIter, edgeComparer);
            if constexpr(clusterEdges)
            {
              upperIter = find_cluster_end(lowerIter, upperIter);
            }

            const auto count{distance(lowerIter, upperIter)};

            if(target == i)
            {
              if(count % 2) throw std::logic_error("Odd number of loop edges");
              if constexpr(!direct_edge_init())
              {
                for(; lowerIter != upperIter; ++lowerIter)
                {
                  if(!(distance(lowerIter, upperIter) % 2) || !EdgeTraits::shared_weight_v)
                  {
                    m_Edges.push_back_to_partition(i, make_edge(i, *lowerIter));
                  }
                  else
                  {
                    const auto compIndex{static_cast<edge_index_type>(distance(orderedEdges.cbegin_partition(i), lowerIter-1))};
                    m_Edges.push_back_to_partition(i, cbegin_edges(i) + compIndex);
                  }
                }
              }
            }
            else
            {
              const auto comparisonEdge{
                [lowerIter,i](){
                  auto compEdge{*lowerIter};
                  compEdge.target_node(i);
                  return compEdge;
                }()
              };

              auto eqrange{
                std::equal_range(orderedEdges.cbegin_partition(target), orderedEdges.cend_partition(target), comparisonEdge, edgeComparer)
              };

              if(eqrange.first == eqrange.second)
                throw std::logic_error("Reciprocated partial edge does not exist");

              if constexpr(clusterEdges)
              {
                while((eqrange.first != eqrange.second) && (eqrange.first->weight() != lowerIter->weight())) ++eqrange.first;

                eqrange.second = find_cluster_end(eqrange.first, eqrange.second);
              }

              if(distance(eqrange.first, eqrange.second) != count)
                throw std::logic_error("Reciprocated target indices do not match");

              if constexpr(!direct_edge_init())
              {
                for(; lowerIter != upperIter; ++lowerIter)
                {
                  if((i < target) || !EdgeTraits::shared_weight_v)
                  {
                    m_Edges.push_back_to_partition(i, make_edge(i, *lowerIter));
                  }
                  else
                  {
                    static_assert(!EdgeTraits::shared_edge_v);

                    const auto compIndex{static_cast<size_t>(distance(orderedEdges.cbegin_partition(target), eqrange.second + distance(upperIter, lowerIter)))};

                    m_Edges.push_back_to_partition(i, edge_type{target, *(cbegin_edges(target) + compIndex)});
                  }
                }
              }
            }

            lowerIter = upperIter;
          }
        }
      }

      template<class EdgeInitializer>
      [[nodiscard]]
      edge_type make_edge([[maybe_unused]] const edge_index_type source, const EdgeInitializer& edgeInit)
      {
        static_assert(!std::is_same_v<edge_type, edge_init_type>, "Logic error!");
        if constexpr(edge_type::flavour == edge_flavour::partial)
        {
          static_assert(!std::is_empty_v<edge_weight_type>);
          return edge_type{edgeInit.target_node(), make_edge_weight(edgeInit.weight())};
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          static_assert(!std::is_empty_v<edge_weight_type>);
          return edge_type{edgeInit.target_node(), edgeInit.complementary_index(), make_edge_weight(edgeInit.weight())};
        }
        else if constexpr(edge_type::flavour == edge_flavour::full)
        {
          using inv_t = inversion_constant<true>;
          if constexpr(std::is_empty_v<edge_weight_type>)
          {
            if(edgeInit.inverted())
              return edge_type{source, inv_t{}};
            else
              return edge_type{source, edgeInit.target_node()};
          }
          else
          {
            if(edgeInit.inverted())
              return edge_type{source, inv_t{}, make_edge_weight(edgeInit.weight())};
            else
              return edge_type{source, edgeInit.target_node(), make_edge_weight(edgeInit.weight())};
          }
        }
        else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
        {
          static_assert(!std::is_empty_v<edge_weight_type>);
          using inv_t = inversion_constant<true>;
          if(edgeInit.inverted())
          {
            return edge_type{edgeInit.source_node(), inv_t{}, edgeInit.complementary_index(), make_edge_weight(edgeInit.weight())};
          }
          else
          {
            return edge_type{edgeInit.source_node(), edgeInit.target_node(), edgeInit.complementary_index(), make_edge_weight(edgeInit.weight())};
          }
        }
      }

      void copy_edges(const connectivity& in)
      {
        if constexpr((edge_type::flavour == edge_flavour::partial) || (edge_type::flavour == edge_flavour::full))
        {
          using edge_weight_proxy = typename edge_type::weight_proxy_type;
          std::map<const edge_weight_proxy*, std::pair<edge_index_type, edge_index_type>> weightMap;
          auto processor{
            [this, &weightMap](const size_type node, const_edge_iterator first, const_edge_iterator i) {
              auto weightPtr{&i->weight_proxy()};
              if(auto found{weightMap.find(weightPtr)}; found == weightMap.end())
              {
                auto appender{[this, node, first, i, &weightMap](auto&& e){
                    m_Edges.push_back_to_partition(node, std::move(e));
                    const std::pair<edge_index_type, edge_index_type> indices{node, static_cast<edge_index_type>(distance(first, i))};
                    weightMap.emplace(&i->weight_proxy(), indices);
                  }
                };

                if constexpr(edge_type::flavour == edge_flavour::partial)
                {
                  edge_type e{i->target_node(), make_edge_weight(i->weight())};
                  appender(std::move(e));
                }
                else
                {
                  if(i->inverted())
                  {
                    using inv_t = inversion_constant<true>;
                    edge_type e{i->source_node(), inv_t{}, make_edge_weight(i->weight())};
                    appender(std::move(e));
                  }
                  else
                  {
                    edge_type e{i->source_node(), i->target_node(), make_edge_weight(i->weight())};
                    appender(std::move(e));
                  }
                }
              }
              else
              {
                const auto& indices{found->second};
                if constexpr(edge_type::flavour == edge_flavour::partial)
                {
                  m_Edges.push_back_to_partition(node, i->target_node(), *(cbegin_edges(indices.first)+indices.second));
                }
                else if constexpr(edge_type::flavour == edge_flavour::full)
                {
                  m_Edges.push_back_to_partition(node, cbegin_edges(indices.first)+indices.second);
                }
              }
            }
          };

          copy_edges(in, processor);
        }
        else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
        {
          auto nodeIndexGetter{
            [](const size_type node, const edge_type& e){
              if(e.source_node() < node) return e.source_node();

              if(e.target_node() < node) return e.target_node();

              return node;
            }
          };

          auto processor{
            [this, nodeIndexGetter](const size_type node, const_edge_iterator first, const_edge_iterator i) {
              const auto dist{static_cast<edge_index_type>(distance(first, i))};
              const bool encountered{((i->source_node() < node) || (i->target_node() < node))
                  || ((i->source_node() == node) && (i->target_node() == node) && (i->complementary_index() < dist))};

              if(!encountered)
              {
                if(i->inverted())
                {
                  using inv_t = inversion_constant<true>;
                  edge_type e{i->source_node(), inv_t{}, i->complementary_index(), make_edge_weight(i->weight())};
                  m_Edges.push_back_to_partition(node, std::move(e));
                }
                else
                {
                  edge_type e{i->source_node(), i->target_node(), i->complementary_index(), make_edge_weight(i->weight())};
                  m_Edges.push_back_to_partition(node, std::move(e));
                }
              }
              else
              {
                const auto compI{i->complementary_index()};
                m_Edges.push_back_to_partition(node, compI, *(cbegin_edges(nodeIndexGetter(node, *i)) + compI));
              }
            }
          };

          copy_edges(in, processor);
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          auto processor{
            [this](const size_type node, const_edge_iterator first, const_edge_iterator i) {
              const auto dist{static_cast<edge_index_type>(distance(first, i))};
              const bool encountered{(i->target_node() < node)
                  || ((i->target_node() == node) && (i->complementary_index() < dist))};

              if(!encountered)
              {
                edge_type e{i->target_node(), i->complementary_index(), make_edge_weight(i->weight())};
                m_Edges.push_back_to_partition(node, std::move(e));
              }
              else
              {
                const auto compI{i->complementary_index()};
                m_Edges.push_back_to_partition(node, i->target_node(), compI, *(cbegin_edges(i->target_node()) + compI));
              }
            }
          };

          copy_edges(in, processor);
        }
        else
        {
          static_assert(dependent_false<edge_type>::value, "Edge flavour not dealt with");
        }
      }

      template<class Processor>
      void copy_edges(const connectivity& in, Processor processor)
      {
        reserve_nodes(in.m_Edges.num_partitions());
        if constexpr(!graph_impl::has_reservable_partitions<edge_storage_type>)
        {
          reserve_edges(in.m_Edges.size());
        }
        for(size_type i{}; i<in.order(); ++i)
        {
          m_Edges.add_slot();
          if constexpr(graph_impl::has_reservable_partitions<edge_storage_type>)
          {
            using std::distance;
            reserve_edges(i, distance(in.cbegin_edges(i), in.cend_edges(i)));
          }
          for(auto inIter{in.cbegin_edges(i)}; inIter != in.cend_edges(i); ++inIter)
          {
            processor(i, in.cbegin_edges(i), inIter);
          }
        }
      }

      [[nodiscard]]
      constexpr auto to_edge_iterator(const_edge_iterator citer)
      {
        const auto source{citer.partition_index()};
        const auto dist{distance(cbegin_edges(source), citer)};
        return m_Edges.begin_partition(source) + dist;
      }

      template<class Setter, class... Args>
      constexpr void manipulate_source_edge_weight(const_edge_iterator citer, Setter setter)
      {
        setter(to_edge_iterator(citer));
      }

      template<class Fn>
      constexpr void mutate_source_edge_weight(const_edge_iterator citer, Fn fn)
      {
        manipulate_source_edge_weight(citer, [fn](const auto& iter){ iter->mutate_weight(fn); });
      }

      template<class... Args>
      constexpr void set_source_edge_weight(const_edge_iterator citer, Args&&... args)
      {
        manipulate_source_edge_weight(citer, [&args...](const auto& iter){ iter->weight(std::forward<Args>(args)...); });
      }

      template<class Setter>
      constexpr const_edge_iterator manipulate_partner_edge_weight(const_edge_iterator citer, Setter setter)
      {
        const auto partner{partner_index(citer)};

        if constexpr(embeddedEdge)
        {
          const auto comp{citer->complementary_index()};
          setter(m_Edges.begin_partition(partner) + comp);
          return m_Edges.cbegin_partition(partner) + comp;
        }
        else
        {
          const auto source{citer.partition_index()};
          auto found{cend_edges(partner)};
          if(source == partner)
          {
            for(auto iter{m_Edges.begin_partition(partner)}; iter != m_Edges.end_partition(partner); ++iter)
            {
              if(distance(m_Edges.begin_partition(partner), iter) == distance(cbegin_edges(partner), citer))
                continue;

              if(*iter == *citer)
              {
                setter(iter);
                found = cbegin_edges(partner) + distance(m_Edges.begin_partition(partner), iter);
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
                setter(iter);
                found = cbegin_edges(partner) + distance(m_Edges.begin_partition(partner), iter);
                break;
              }
            }
          }

          if(found != cend_edges(partner))
          {
            return found;
          }
          else
          {
            throw std::logic_error("Partner weight not found");
          }
        }
      }

      template<class Fn>
      constexpr void mutate_partner_edge_weight(const_edge_iterator citer, Fn fn)
      {
        manipulate_partner_edge_weight(citer, [fn](const auto& iter){ iter->mutate_weight(fn); });
      }

      template<class... Args>
      constexpr const_edge_iterator set_partner_edge_weight(const_edge_iterator citer, Args&&... args)
      {
        return manipulate_partner_edge_weight(citer, [&args...](const auto& iter){ iter->weight(std::forward<Args>(args)...); });
      }

      template<class... Args>
      const_edge_iterator insert_single_join(const_edge_iterator citer1, const edge_index_type node2, const edge_index_type pos2, Args&&... args)
      {
        if constexpr (std::is_empty_v<edge_weight_type>)
          static_assert(sizeof...(args) == 0, "Makes no sense to supply arguments for an empty weight!");

        const auto node1{citer1.partition_index()};
        if constexpr(std::is_empty_v<edge_weight_type>)
        {
          if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            citer1 = m_Edges.insert_to_partition(citer1, node2, pos2);
            increment_comp_indices(++to_edge_iterator(citer1), end_edges(node1), 1);

          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            const auto pos1{static_cast<edge_index_type>(distance(cbegin_edges(node1), citer1))};
            if((node1 == node2) && (pos2 <= pos1))
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, inversion_constant<true>{});
            }
            else
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, node2);
            }
          }
          else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
          {
            citer1 = m_Edges.insert_to_partition(citer1, node1, node2, pos2);
            increment_comp_indices(++to_edge_iterator(citer1), end_edges(node1), 1);
          }
        }
        else
        {
          if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            citer1 = m_Edges.insert_to_partition(citer1, node2, pos2, make_edge_weight(std::forward<Args>(args)...));
            increment_comp_indices(++to_edge_iterator(citer1), end_edges(node1), 1);
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            const auto pos1{static_cast<edge_index_type>(distance(cbegin_edges(node1), citer1))};
            if((node1 == node2) && (pos2 <= pos1))
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, inversion_constant<true>{}, make_edge_weight(std::forward<Args>(args)...));
            }
            else
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, node2, make_edge_weight(std::forward<Args>(args)...));
            }
          }
          if constexpr(edge_type::flavour == edge_flavour::full_embedded)
          {
            citer1 = m_Edges.insert_to_partition(citer1, node1, node2, pos2, make_edge_weight(std::forward<Args>(args)...));
            increment_comp_indices(++to_edge_iterator(citer1), end_edges(node1), 1);
          }
        }

        return citer1;
      }

      template<std::input_or_output_iterator Iter, class Fn> constexpr void modify_comp_indices(Iter first, Iter last, Fn fn) noexcept
      {
        const auto start{first};
        while(first != last)
        {
          const auto source{first.partition_index()};
          const auto other{[first]([[maybe_unused]] const auto n){
              const auto target{first->target_node()};
              if constexpr(edge_type::flavour == edge_flavour::full_embedded)
              {
                return n == target ? first->source_node() : target;
              }
              else
              {
                return target;
              }
            }(source)
          };

          const auto compIndex{first->complementary_index()};
          if(source != other)
          {
            const auto edgeIter{m_Edges.begin_partition(other) + compIndex};
            edgeIter->complementary_index(fn(edgeIter->complementary_index()));
          }
          else
          {
            const auto startPos{static_cast<edge_index_type>(distance(begin_edges(source), start))};
            if(const auto shiftedCompIndex{fn(compIndex)}; shiftedCompIndex >= startPos)
            {
              first->complementary_index(shiftedCompIndex);
            }
            else if(const auto edgeIter{m_Edges.begin_partition(other) + compIndex};
                    fn(edgeIter->complementary_index()) >= startPos)
            {
              edgeIter->complementary_index(fn(edgeIter->complementary_index()));
            }
          }

          ++first;
        }
      }

      template<std::input_or_output_iterator Iter> constexpr void decrement_comp_indices(Iter first, Iter last, const edge_index_type num) noexcept
      {
        auto decrementer{
          [num](const auto compIndex){
            return compIndex >= num ? compIndex - num : edge_index_type{};
          }
        };
        modify_comp_indices(first, last, decrementer);
      }

      template<std::input_or_output_iterator Iter> constexpr void increment_comp_indices(Iter first, Iter last, const edge_index_type num) noexcept
      {
        modify_comp_indices(first, last, [num](const auto compIndex) { return compIndex + num; });
      }

      template<class Pred, class Modifier>
      constexpr void fix_edge_data(const edge_index_type node, Pred pred, Modifier modifier) noexcept
      {
        for(size_type i{}; i < m_Edges.num_partitions(); ++i)
        {
          for(auto iter{m_Edges.begin_partition(i)}; iter != m_Edges.end_partition(i); ++iter)
          {
            const auto target{iter->target_node()};
            if constexpr(EdgeTraits::shared_edge_v)
            {
              const auto source{iter->source_node()}, currentNode{iter.partition_index()};

              if(pred(target, node))
              {
                if(const auto newTarget{modifier(target)}; newTarget == currentNode) iter->target_node(newTarget);
              }
              if(pred(source, node))
              {
                if(const auto newHost{modifier(source)}; newHost == currentNode) iter->source_node(newHost);
              }
            }
            else
            {
              if(pred(target, node)) iter->target_node(modifier(target));

              if constexpr(edge_type::flavour == edge_flavour::full_embedded)
              {
                const auto source{iter->source_node()};
                if(pred(source, node)) iter->source_node(modifier(source));
              }
            }
          }
        }
      }
    };
  }
}
