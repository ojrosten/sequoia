////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file Connectivity.hpp
    \brief Implementation for the storage of graph edges.
 */

#include "DataStructuresTypeTraits.hpp"
#include "GraphDetails.hpp"
#include "Algorithms.hpp"
#include "TypeTraits.hpp"
#include "AssignmentUtilities.hpp"

#include <limits>
#include <set>
#include <map>

namespace sequoia
{
  namespace data_structures
  {
    template <class, class, class> class bucketed_storage;
    template <class, class> struct bucketed_storage_traits;
    template <class, class, class> class contiguous_storage;
    template <class, class> struct contiguous_storage_traits;
    template <class, std::size_t, std::size_t, class> class static_contiguous_storage;
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

      using edge_type        = typename EdgeTraits::edge_type;
      using edge_weight_type = typename edge_type::weight_type;
      using edge_index_type  = typename edge_type::index_type;
      using edge_init_type   = typename EdgeTraits::edge_init_type;
      using size_type        = typename edge_storage_type::size_type;

      static_assert(std::is_unsigned_v<edge_index_type>);
      
      using const_edge_iterator = typename edge_storage_type::const_partition_iterator;
      using const_reverse_edge_iterator = typename edge_storage_type::const_reverse_partition_iterator;
    
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
      constexpr size_type size()  const noexcept
      {
        auto size{m_Edges.size()};
        if constexpr (EdgeTraits::mutual_info_v) return size /= 2;

        return size;
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
          mutate_host_edge_weight(citer, fn);          
        }
        else
        {
          mutate_host_edge_weight(citer, fn);
        }
      }
     
      template<class Fn>
      constexpr void mutate_edge_weight(const_reverse_edge_iterator criter, Fn fn)
      {
        const auto host{criter.partition_index()};
        mutate_edge_weight(m_Edges.cbegin_partition(host) + distance(criter, m_Edges.crend_partition(host)) - 1, fn);
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

            auto hostSetter{
              [this](const_edge_iterator citer, const_edge_iterator partnerIter){
                this->set_host_edge_weight(citer, partnerIter->weight());
              }
            };

            weight_sentinel sentinel{*this, citer, partnerSetter, hostSetter,
                                            std::forward<Arg>(arg), std::forward<Args>(args)...};
          }
          else
          {
            auto partnerIter{set_partner_edge_weight(citer, std::forward<Arg>(arg), std::forward<Args>(args)...)};
            set_host_edge_weight(citer, partnerIter->weight());
          }
        }
        else
        {
          set_host_edge_weight(citer, std::forward<Arg>(arg), std::forward<Args>(args)...);
        }
      }

      template<class Arg, class... Args>
      constexpr void set_edge_weight(const_reverse_edge_iterator criter, Arg&& arg, Args&&... args)
      {
        const auto host{criter.partition_index()};
        set_edge_weight(m_Edges.cbegin_partition(host) + distance(criter, m_Edges.crend_partition(host)) - 1, std::forward<Arg>(arg), std::forward<Args>(args)...);
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

      template<class Allocator, class... Allocators>
      constexpr connectivity(const Allocator& a, const Allocators&... as)
        : m_Edges(a, as...)
      {}

      template<class Allocator, class... Allocators>
      constexpr connectivity(init_t edges, const Allocator& a, const Allocators&... as)
        : connectivity(direct_edge_init(), edges, a, as...)
      {}
      
      template<class Allocator, class... Allocators>
      constexpr connectivity(const connectivity& c, const Allocator& a, const Allocators&... as)
        : connectivity(direct_edge_copy(), c, a, as...)
      {}
      
      constexpr connectivity(connectivity&&) noexcept = default;

      template<class Allocator, class... Allocators>
      constexpr connectivity(connectivity&& c, const Allocator& a, const Allocators&... as)
        : m_Edges{std::move(c.m_Edges), a, as...}
      {}

      ~connectivity() = default;

      constexpr connectivity& operator=(connectivity&&) = default;
      
      constexpr connectivity& operator=(const connectivity& in)
      {
        if(&in != this)
        {
          auto partitionsAllocGetter{
            [](const connectivity& in){
              if constexpr(has_partitions_allocator_type_v<edge_storage_type>)
              {
                return in.m_Edges.get_partitions_allocator();
              }
            }
          };
          
          auto allocGetter{
            [](const connectivity& in){
              if constexpr(has_allocator_type_v<edge_storage_type>)
              {
                return in.m_Edges.get_allocator();
              }
            }
          };

          sequoia::impl::assignment_helper::assign(*this, in, partitionsAllocGetter, allocGetter);
        }

        return *this;
      }

      void swap(connectivity& other)
        noexcept(noexcept(m_Edges, other.m_Edges))
      {
        sequoia::swap(m_Edges, other.m_Edges);
      }

      constexpr void swap_edges(edge_index_type node, edge_index_type i, edge_index_type j)
      {
        sequoia::iter_swap(begin_edges(node) + i, begin_edges(node) + j);
      }

      constexpr void swap_nodes(size_type i, size_type j)
      {
        if constexpr (EdgeTraits::shared_edge_v)
        {
          for(auto iter{begin_edges(i)}; iter != end_edges(i); ++iter)
          {
            if((iter->host_node() != j) && (iter->target_node() != j))
            {
              if(iter->host_node() == i) iter->host_node(j);
              else if(iter->host_node() == j) iter->host_node(i);
              
              if(iter->target_node() == i) iter->target_node(j);
              else if(iter->target_node() == j) iter->target_node(i);
            }
          }

          for(auto iter{begin_edges(j)}; iter != end_edges(j); ++iter)
          {
            if(iter->host_node() == i) iter->host_node(j);
            else if(iter->host_node() == j) iter->host_node(i);
            
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
                if(iter->host_node() == i) iter->host_node(j);
                else if(iter->host_node() == j) iter->host_node(i);
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

      template
      <
        class T=edge_storage_type,
        std::enable_if_t<has_partitions_allocator_type_v<T>, int> = 0
      >
      auto get_edge_allocator(partitions_allocator_tag) const
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

      template<class T=edge_storage_type>
      std::enable_if_t<graph_impl::has_reservable_partitions_v<T>>
      reserve_edges(const edge_index_type partition, const edge_index_type size)
      {
        m_Edges.reserve_partition(partition, size);
      }

      template<class T=edge_storage_type>
      std::enable_if_t<!graph_impl::has_reservable_partitions_v<T>>
      reserve_edges(const edge_index_type size)
      {
        m_Edges.reserve(size);
      }

      template<class T=edge_storage_type>
      [[nodiscard]]
      std::enable_if_t<graph_impl::has_reservable_partitions_v<T>, size_type>
      edges_capacity(const edge_index_type partition) const
      {
        return m_Edges.partition_capacity(partition);
      }

      template<class T=edge_storage_type>
      [[nodiscard]]
      std::enable_if_t<!graph_impl::has_reservable_partitions_v<T>, size_type>
      edges_capacity() const noexcept
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
              const auto host{citer->host_node()};
              if(host != node) partitionsToVisit.insert(host);
            }
          }

          for(const auto partition : partitionsToVisit)
          {
            auto fn = [node](const edge_type& e) {
              if constexpr (directed(directedness))
              {
                return (e.target_node() == node) || (e.host_node() == node);
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

      void reserve_for_join(const edge_index_type node1, const edge_index_type node2)
      {
        if constexpr (graph_impl::has_reservable_partitions_v<edge_storage_type>)
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
          const auto host{citer.partition_index()};
          const auto partner{partner_index(citer)};

          if(host != partner)
          {                        
            if constexpr (embeddedEdge)
            {
              const auto partnerLocalIndex{citer->complementary_index()};
              citer = m_Edges.erase_from_partition(citer);
              decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(host), 1);

              auto partnerIter{m_Edges.erase_from_partition(partner, partnerLocalIndex)};
              decrement_comp_indices(to_edge_iterator(partnerIter), m_Edges.end_partition(partner), 1);              
            }
            else
            {
              const auto partnerDist{[this, partner](const auto host, const auto citer){
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
                      if(oiter->target_node() == host)
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
                }(host, citer)};
                            
              m_Edges.erase_from_partition(citer);              
              m_Edges.erase_from_partition(cbegin_edges(partner) + partnerDist);
            }
          }
          else
          {            
            if constexpr (embeddedEdge)
            {
              const auto hostLocalIndex{citer->complementary_index()};
              const auto dist = distance(citer, cbegin_edges(host) + hostLocalIndex); 
              if(dist == 1)
              {
                citer = m_Edges.erase_from_partition(citer);
                citer = m_Edges.erase_from_partition(citer);
                
                decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(host), 2);
              }
              else if(dist == -1)
              {
                citer = m_Edges.erase_from_partition(citer-1);
                citer = m_Edges.erase_from_partition(citer);
                
                decrement_comp_indices(to_edge_iterator(citer), m_Edges.end_partition(host), 2);
              }
              else
              {                
                const auto eraseFirstIndex{(hostLocalIndex > dist) ? hostLocalIndex : dist};
                const auto eraseSecondIndex{(hostLocalIndex > dist) ? dist : hostLocalIndex};

                auto firstIter{m_Edges.begin_partition(host) + eraseFirstIndex};
                firstIter = m_Edges.erase_from_partition(firstIter);
                
                auto secondIter{m_Edges.begin_partition(host) + eraseSecondIndex};
                secondIter = m_Edges.erase_from_partition(secondIter);
                firstIter = m_Edges.begin_partition(host) + eraseFirstIndex;

                decrement_comp_indices(firstIter, m_Edges.end_partition(host), 2);                
                decrement_comp_indices(secondIter, firstIter, 1);
              }
            }
            else if constexpr (EdgeTraits::shared_edge_v)
            {
              auto pEdge{&*citer};
              auto hiter{cbegin_edges(host)};
              while(hiter != cend_edges(host))
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
              auto dist{distance(cbegin_edges(host), citer)};
              for(auto ci{cbegin_edges(host)}; ci != cend_edges(host); ++ci)
              {
                if((ci != citer) && (ci->target_node() == host))
                {
                  const bool remove{[](const auto ci, const auto citer){
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
                    if(distance(cbegin_edges(host), ci) < dist) --dist;
                  
                    m_Edges.erase_from_partition(ci);
                    break;
                  }
                }
              }              

              m_Edges.erase_from_partition(cbegin_edges(host) + dist);                 
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
        if(begin != end)
        {
          auto bhost{begin.partition_index()}, ehost{end.partition_index()};
          if(bhost == ehost)
          {
            sequoia::sort(begin_edges(bhost) + distance(cbegin_edges(bhost), begin), begin_edges(ehost) + distance(cbegin_edges(ehost), end), comp);
          }
          else
          {
            if(bhost > ehost) std::swap(bhost, ehost);
            sequoia::sort(begin_edges(bhost) + distance(cbegin_edges(bhost), begin), end_edges(bhost), comp);
            for(auto i{bhost+1}; i<ehost; ++i)
            {
              sequoia::sort(begin_edges(i), end_edges(i), comp);
            }
            sequoia::sort(begin_edges(ehost), begin_edges(ehost) + distance(cbegin_edges(ehost), end), comp);
          }
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
      
      static constexpr bool protectiveEdgeWeightProxy{
        std::is_same_v<typename edge_type::weight_proxy_type, utilities::protective_wrapper<edge_weight_type>>
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
      template<class... Allocators>
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

      template
      <        
        class EdgeAllocator,
        class PartitionAllocator,
        std::enable_if_t<is_constructible_with_v<edge_storage_type, EdgeAllocator, PartitionAllocator>, int> = 0
      >
      constexpr connectivity(indirect_edge_init_type, init_t edges, const EdgeAllocator& edgeAllocator, const PartitionAllocator& partitionAllocator)
        : m_Edges(edgeAllocator, partitionAllocator)
      {
         process_edges(edges);
      }

      template<class EdgeAllocator>
      constexpr connectivity(indirect_edge_init_type, init_t edges, const EdgeAllocator& edgeAllocator)
        : m_Edges(edgeAllocator)
      {
         process_edges(edges);
      }

      template<class... Allocators>
      constexpr connectivity(direct_edge_copy_type, const connectivity& in, const Allocators&... as)
        : m_Edges{in.m_Edges, as...}
      {}

      constexpr connectivity(indirect_edge_copy_type, const connectivity& in)
        : m_Edges{}
      {
        copy_edges(in);
      }

      template
      <
        class PartitionAllocator,
        class... EdgeAllocators,
        std::enable_if_t<is_constructible_with_v<edge_storage_type, PartitionAllocator, EdgeAllocators...>, int> = 0
      >
      constexpr connectivity(indirect_edge_copy_type, const connectivity& in, const PartitionAllocator& partitionAllocator, const EdgeAllocators&... edgeAllocators)
        : m_Edges(partitionAllocator, edgeAllocators...)
      {
         copy_edges(in);
      }

      template
      <
        class PartitionAllocator,
        class... EdgeAllocators,
        std::enable_if_t<!is_constructible_with_v<edge_storage_type, PartitionAllocator, EdgeAllocators...>, int> = 0
      >
      constexpr connectivity(indirect_edge_copy_type, const connectivity& in, const PartitionAllocator& partitionAllocator, const EdgeAllocators&... edgeAllocators)
        : m_Edges(partitionAllocator)
      {
         copy_edges(in, edgeAllocators...);
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
          const auto host{citer.partition_index()};
          return target == host ? edge.host_node() : target;
        }
        else
        {
          return target;
        }
      }

      constexpr void process_edges(std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
        if constexpr (EdgeTraits::init_complementary_data_v)
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
                const auto host{std::distance(edges.begin(), nodeEdgesIter)};
                m_Edges.push_back_to_partition(host, edge_type{edge.target_node(), make_edge_weight(edge.weight())});
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
          const auto currentNodeIndex{std::distance(edges.begin(), nodeEdgesIter)};
          for(auto edgeIter{nodeEdges.begin()}; edgeIter != nodeEdges.end(); ++edgeIter)
          {
            const auto& edge{*edgeIter};
            const auto target{edge.target_node()};
            if(target >= edges.size()) throw std::logic_error("Target index out of range");
                   
            const auto compIndex{edge.complementary_index()};

            bool doProcess{true};
            if constexpr(directed(directedness))
            {
              doProcess = (edge.target_node() != currentNodeIndex) || (edge.host_node() == edge.target_node());
            }
             
            if(doProcess)
            {
              auto targetEdgesIter = edges.begin() + target;  
              if(compIndex >= targetEdgesIter->size())
                throw std::logic_error("Complementary index out of range");
               
              if((target == currentNodeIndex) && (compIndex == std::distance(nodeEdges.begin(), edgeIter)))
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

                if(edge.host_node() != targetEdge.host_node())
                  throw std::logic_error("Reciprocated host index does not match for embedded directed graph");
              }
               
              if(targetEdge.complementary_index() != std::distance(nodeEdges.begin(), edgeIter))
                throw std::logic_error("Reciprocated complementary index does not match" );

              if constexpr (!std::is_empty_v<edge_weight_type>)
              {
                if(edge.weight() != targetEdge.weight())
                  throw std::logic_error("Mismath between weights");
              }
            }

            if constexpr(directed(directedness))
            {               
              const auto host{edge.host_node()};
              if(host >= edges.size()) throw std::logic_error("Host index out of range");
              if((host != currentNodeIndex) && (target != currentNodeIndex)) throw std::logic_error("At least one of host and target must match current node");
              
              if((edge.target_node() == currentNodeIndex) && (edge.host_node() != edge.target_node()))
              {  
                auto hostEdgesIter = edges.begin() + host;
                if(compIndex >= hostEdgesIter->size()) throw std::logic_error("Complementary index out of range");
                
                const auto& hostEdge = *(hostEdgesIter->begin() + compIndex);
                if(hostEdge.target_node() != currentNodeIndex)
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
                const auto host{edge.host_node()};
                const auto partner{target == currentNodeIndex ? host : target};
                if((partner > currentNodeIndex) || ((partner == currentNodeIndex) && (compIndex > pos)))
                {
                  m_Edges.push_back_to_partition(currentNodeIndex, make_edge(host, edge));
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
        using traits_t = contiguous_storage_traits<edge_init_type, data_sharing::independent<edge_init_type>>;
        contiguous_storage<edge_init_type, data_sharing::independent<edge_init_type>, traits_t> edgesForChecking{edges};
        process_edges(edgesForChecking);
      }

      constexpr void process_edges(direct_edge_init_type, init_t edges)
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
      
      template<class Edges, class... Allocators>
      constexpr void process_edges(Edges& orderedEdges, const Allocators&... allocs)
      {
        constexpr bool sortWeights{!std::is_empty_v<edge_weight_type> && is_orderable_v<edge_weight_type>};
        constexpr bool clusterEdges{!std::is_empty_v<edge_weight_type> && !is_orderable_v<edge_weight_type>};

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
        
        for(size_type i{}; i< orderedEdges.num_partitions(); ++i)
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

           
        for(size_type i{}; i<orderedEdges.num_partitions(); ++i)
        {
          if constexpr(!direct_edge_init_v) m_Edges.add_slot(allocs...);
          
          auto lowerIter{orderedEdges.cbegin_partition(i)}, upperIter{orderedEdges.cbegin_partition(i)};
          while(lowerIter != orderedEdges.cend_partition(i))
          {
            const auto target{lowerIter->target_node()};
            if(target >= orderedEdges.num_partitions())
              throw std::logic_error("Target index out of range");

            upperIter = sequoia::upper_bound(lowerIter, orderedEdges.cend_partition(i), *lowerIter, edgeComparer);
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

              auto range{
                sequoia::equal_range(orderedEdges.cbegin_partition(target), orderedEdges.cend_partition(target), comparisonEdge, edgeComparer)
              };

              if(range.first == range.second)
                throw std::logic_error("Reciprocated partial edge does not exist");

              if constexpr(clusterEdges)
              {
                while((range.first->weight() != lowerIter->weight()) && (range.first != range.second)) ++range.first;

                range.second = find_cluster_end(range.first, range.second);
              }

              if(distance(range.first, range.second) != count)
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
                    
                    const auto compIndex{static_cast<size_t>(distance(orderedEdges.cbegin_partition(target), range.second + distance(upperIter, lowerIter)))};

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
      edge_type make_edge(const edge_index_type host, const EdgeInitializer& edgeInit)
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
              return edge_type{host, inv_t{}};
            else
              return edge_type{host, edgeInit.target_node()};
          }
          else
          {
            if(edgeInit.inverted())
              return edge_type{host, inv_t{}, make_edge_weight(edgeInit.weight())};
            else
              return edge_type{host, edgeInit.target_node(), make_edge_weight(edgeInit.weight())};
          }
        }
        else if constexpr(edge_type::flavour == edge_flavour::full_embedded)
        {
          static_assert(!std::is_empty_v<edge_weight_type>);
          using inv_t = inversion_constant<true>;
          if(edgeInit.inverted())
          {
            return edge_type{edgeInit.host_node(), inv_t{}, edgeInit.complementary_index(), make_edge_weight(edgeInit.weight())};
          }
          else
          {            
            return edge_type{edgeInit.host_node(), edgeInit.target_node(), edgeInit.complementary_index(), make_edge_weight(edgeInit.weight())};
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
                    edge_type e{i->host_node(), inv_t{}, make_edge_weight(i->weight())};
                    appender(std::move(e));
                  }
                  else
                  {
                    edge_type e{i->host_node(), i->target_node(), make_edge_weight(i->weight())};
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
              if(e.host_node() < node) return e.host_node();
              
              if(e.target_node() < node) return e.target_node();

              return node;
            }
          };
            
          auto processor{
            [this, nodeIndexGetter](const size_type node, const_edge_iterator first, const_edge_iterator i) {
              const bool encountered{((i->host_node() < node) || (i->target_node() < node))
                  || ((i->host_node() == node) && (i->target_node() == node) && (i->complementary_index() < distance(first, i)))};

              if(!encountered)
              {
                if(i->inverted())
                {                  
                  using inv_t = inversion_constant<true>;
                  edge_type e{i->host_node(), inv_t{}, i->complementary_index(), make_edge_weight(i->weight())};
                  m_Edges.push_back_to_partition(node, std::move(e));
                }
                else
                {
                  edge_type e{i->host_node(), i->target_node(), i->complementary_index(), make_edge_weight(i->weight())};
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
              const bool encountered{(i->target_node() < node)
                  || ((i->target_node() == node) && (i->complementary_index() < distance(first, i)))};

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

      template<class Processor, class... Allocators> 
      void copy_edges(const connectivity& in, Processor processor, const Allocators&... allocs)
      {
        for(size_type i{}; i<in.order(); ++i)
        {
          m_Edges.add_slot(allocs...);
          for(auto inIter{in.cbegin_edges(i)}; inIter != in.cend_edges(i); ++inIter)
          {
            processor(i, in.cbegin_edges(i), inIter);
          }
        }
      }

      [[nodiscard]]
      constexpr auto to_edge_iterator(const_edge_iterator citer)
      {
        const auto host{citer.partition_index()};
        const auto dist{distance(cbegin_edges(host), citer)};
        return m_Edges.begin_partition(host) + dist;
      }
      
      template<class Setter, class... Args>
      constexpr void manipulate_host_edge_weight(const_edge_iterator citer, Setter setter)
      {        
        setter(to_edge_iterator(citer));
      }

      template<class Fn>
      constexpr void mutate_host_edge_weight(const_edge_iterator citer, Fn fn)
      {
        manipulate_host_edge_weight(citer, [fn](const auto& iter){ iter->mutate_weight(fn); });
      }
      
      template<class... Args>
      constexpr void set_host_edge_weight(const_edge_iterator citer, Args&&... args)
      {
        manipulate_host_edge_weight(citer, [&args...](const auto& iter){ iter->weight(std::forward<Args>(args)...); });
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
          const auto host{citer.partition_index()};
          if(host == partner)
          {
            for(auto iter{m_Edges.begin_partition(partner)}; iter != m_Edges.end_partition(partner); ++iter)
            {
              if(distance(m_Edges.begin_partition(partner), iter) == distance(cbegin_edges(partner), citer))
                continue;
              
              if(*iter == *citer)
              {
                setter(iter);
                return cbegin_edges(partner) + distance(m_Edges.begin_partition(partner), iter);
              }
            }
          }
          else
          {
            for(auto iter{m_Edges.begin_partition(partner)}; iter != m_Edges.end_partition(partner); ++iter)
            {
              if((iter->target_node() == host) && (iter->weight() == citer->weight()))
              {
                setter(iter);
                return cbegin_edges(partner) + distance(m_Edges.begin_partition(partner), iter);
              }
            }
          }          

          throw std::logic_error("Partner weight not found");
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
        
        const auto node1{citer1.partition_index()}, pos1{static_cast<edge_index_type>(distance(cbegin_edges(node1), citer1))};
        if constexpr(std::is_empty_v<edge_weight_type>)
        {          
          if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            citer1 = m_Edges.insert_to_partition(citer1, node2, pos2);
            increment_comp_indices(++to_edge_iterator(citer1), end_edges(node1), 1);
            
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
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

      template<class Iter, class Fn> constexpr void modify_comp_indices(Iter first, Iter last, Fn fn) noexcept
      {
        const auto start{first};
        while(first != last)
        {
          const auto host{first.partition_index()};
          const auto other{[first](const auto n){
              const auto target{first->target_node()};
              if constexpr(edge_type::flavour == edge_flavour::full_embedded)
              {
                return n == target ? first->host_node() : target;
              }
              else
              {
                return target;
              }
            }(host)
          };

          const auto compIndex{first->complementary_index()};
          if(host != other)
          {
            const auto compIndex{first->complementary_index()};
            const auto edgeIter{m_Edges.begin_partition(other) + compIndex};
            edgeIter->complementary_index(fn(edgeIter->complementary_index()));
          }
          else
          {            
            const auto startPos{static_cast<edge_index_type>(distance(begin_edges(host), start))};
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

      template<class Iter> constexpr void decrement_comp_indices(Iter first, Iter last, const edge_index_type num) noexcept
      {
        auto decrementer{
          [num](const auto compIndex){
            return compIndex >= num ? compIndex - num : edge_index_type{};
          }
        };
        modify_comp_indices(first, last, decrementer); 
      }
      
      template<class Iter> constexpr void increment_comp_indices(Iter first, Iter last, const edge_index_type num) noexcept
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
              const auto host{iter->host_node()}, currentNode{iter.partition_index()};
              
              if(pred(target, node))
              {
                if(const auto newTarget{modifier(target)}; newTarget == currentNode) iter->target_node(newTarget);
              }
              if(pred(host, node))
              {
                if(const auto newHost{modifier(host)}; newHost == currentNode) iter->host_node(newHost);
              }
            }
            else
            {
              if(pred(target, node)) iter->target_node(modifier(target));

              if constexpr(edge_type::flavour == edge_flavour::full_embedded)
              {
                const auto host{iter->host_node()};                
                if(pred(host, node)) iter->host_node(modifier(host));
              }
            }
          }
        }
      }
    };    
  }
}
