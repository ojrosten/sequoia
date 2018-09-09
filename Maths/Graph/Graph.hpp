#pragma once

#include "GraphHelper.hpp"
#include "NodeStorage.hpp"
#include "Algorithms.hpp"

#include <limits>
#include <set>

namespace sequoia
{
  namespace concurrency  
  {
    template<class> class serial;
  }

  namespace data_structures
  {
    template <class, class, bool, template<class...> class> class bucketed_storage;
    template <class, class, bool, template<class...> class> class contiguous_storage;
    template <class, std::size_t, std::size_t, bool, class> class static_contiguous_storage;
    class static_data_base;
  }

  namespace maths
  {
    template
    <      
      directed_flavour Directedness,
      class Nodes,
      class EdgeTraits,
      class WeightMaker
    >
    class graph_primitive : private WeightMaker, public Nodes
    {
    protected:
      using edge_storage_type = typename EdgeTraits::edge_storage_type;
    public:

      using edge_type        = typename EdgeTraits::edge_type;
      using node_weight_type = typename Nodes::weight_type;      
      using edge_weight_type = typename edge_type::weight_type;
      using edge_index_type  = typename edge_type::index_type;
      using edge_init_type   = typename EdgeTraits::edge_init_type;
      using size_type        = typename graph_impl::size_type_generator<edge_storage_type, Nodes>::size_type;

      static_assert(std::is_unsigned_v<edge_index_type>);
      
      using const_edge_iterator = typename edge_storage_type::const_partition_iterator;
      using const_reverse_edge_iterator = typename edge_storage_type::const_reverse_partition_iterator;
    
      constexpr static auto npos{std::numeric_limits<edge_index_type>::max()};
      constexpr static directed_flavour directedness{Directedness};
      constexpr static bool store_incident_edges_v{EdgeTraits::mutual_info_v};
      
      constexpr graph_primitive() noexcept {}
      
      constexpr graph_primitive(std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : graph_primitive(direct_edge_init(), direct_node_init(), edges)
      {
        static_assert(std::is_empty_v<node_weight_type> || std::is_default_constructible_v<node_weight_type>);
      }

      template<class N=node_weight_type, class=std::enable_if_t<!std::is_empty_v<N>>>
      constexpr graph_primitive(std::initializer_list<N> nodeWeights, std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : graph_primitive(direct_edge_init(), direct_node_init(), edges, nodeWeights)
      {
        if(nodeWeights.size() != edges.size())
          throw std::logic_error("Node weight initializer and edges top-level initializer must be of same size");
      }

      constexpr graph_primitive(const graph_primitive& in)
        : graph_primitive(std::bool_constant<direct_edge_init().value
                          || (EdgeTraits::shared_edge_v && std::is_same_v<typename edge_type::weight_proxy_type, utilities::protective_wrapper<edge_weight_type>>)>{}, in)
      {}

      constexpr graph_primitive(graph_primitive&& in) noexcept = default;

      constexpr graph_primitive& operator=(graph_primitive&& in) noexcept = default;
      
      constexpr graph_primitive& operator=(const graph_primitive& in)
      {
        auto tmp = in;
        swap(tmp);
        return *this;
      }

      constexpr size_type order() const noexcept { return m_Edges.num_partitions(); }

      constexpr size_type size()  const noexcept
      {
        auto size{m_Edges.size()};
        if constexpr (EdgeTraits::mutual_info_v) return size /= 2;

        return size;
      }
    
      constexpr const_edge_iterator cbegin_edges(const edge_index_type node) const
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Node index out of range!");
        
        return m_Edges.cbegin_partition(node);
      }

      constexpr const_edge_iterator cend_edges(const edge_index_type node) const
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.cend_partition(node);
      }

      constexpr const_reverse_edge_iterator crbegin_edges(const edge_index_type node) const
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.crbegin_partition(node);
      }

      constexpr const_reverse_edge_iterator crend_edges(const edge_index_type node) const
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.crend_partition(node);
      }
      
      //===============================equality (not isomorphism) operators================================//
      
      friend constexpr bool operator==(const graph_primitive& first, const graph_primitive& second)
      {
        return (static_cast<Nodes&>(first) == static_cast<Nodes&>(second)) && (first.m_Edges == second.m_Edges);
      }

      friend constexpr bool operator!=(const graph_primitive& first, const graph_primitive& second)
      {
        return !(first == second);
      }
    protected:
      using edge_iterator = typename edge_storage_type::partition_iterator;
      
      ~graph_primitive() = default;
      
      constexpr void swap(graph_primitive& rhs) noexcept
      {
        auto tmp = std::move(rhs);
        rhs = std::move(*this);
        *this = std::move(tmp);
      }

      void reserve_nodes(const size_type size)
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::reserve_nodes(size);
        }
        
        m_Edges.reserve_partitions(size);
      }

      size_type node_capacity() const noexcept
      {        
        return m_Edges.num_partitions_capacity();
      }

      template<class T=edge_storage_type>
      std::enable_if_t<graph_impl::has_reservable_partitions_v<T>>
      reserve_edges(const edge_index_type partition, const edge_index_type size)
      {
        m_Edges.reserve(partition, size);
      }

      template<class T=edge_storage_type>
      std::enable_if_t<!graph_impl::has_reservable_partitions_v<T>>
      reserve_edges(const edge_index_type size)
      {
        m_Edges.reserve(size);
      }

      template<class T=edge_storage_type>
      std::enable_if_t<graph_impl::has_reservable_partitions_v<T>>
      edges_capacity(const edge_index_type partition) const
      {
        return m_Edges.capacity(partition);
      }

      template<class T=edge_storage_type>
      std::enable_if_t<graph_impl::has_reservable_partitions_v<T>>
      edges_capacity() const noexcept
      {
        return m_Edges.capacity();
      }
      
      void shrink_to_fit()
      {
        if constexpr(!std::is_empty_v<node_weight_type>)
        {
          Nodes::shrink_to_fit();
        }

        m_Edges.shrink_to_fit();
      }

      constexpr edge_iterator begin_edges(const edge_index_type node)
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Node index out of range!");
        
        return m_Edges.begin_partition(node);
      }

      constexpr edge_iterator end_edges(const edge_index_type node)
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Node index out of range!");

        return m_Edges.end_partition(node);
      }
      
      template<class... Args>
      size_type add_node(Args&&... args)
      {
        m_Edges.add_slot();
        if constexpr (!emptyNodes) Nodes::add_node(WeightMaker::make_node_weight(std::forward<Args>(args)...));
        return (order()-1);
      }

      template<class... Args>
      size_type insert_node(const size_type pos, Args&&... args)
      {        
        const auto node = (pos < order()) ? pos : (order() - 1);
        if constexpr (!emptyNodes) Nodes::insert_node(this->cbegin_node_weights() + pos, WeightMaker::make_node_weight(std::forward<Args>(args)...));
        m_Edges.insert_slot(node);
        fix_edge_data(node,
                      [](const auto targetNode, const auto node) { return targetNode >= node; },
                      [](const auto index) { return index + 1; });

        return node;
      }

      void delete_node(const size_type node)
      {
        if constexpr (throwOnRangeError) if(node >= order()) throw std::out_of_range("Cannot delete node: index out of range");

        if constexpr (EdgeTraits::mutual_info_v)
        {
          std::set<size_type> partitionsToVisit;
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

            if constexpr (edge_type::flavour == edge_flavour::partial_embedded)
            {
              for(auto iter{m_Edges.begin_partition(partition)}; iter != m_Edges.end_partition(partition); ++iter)
              {
                if(fn(*iter)) decrement_comp_indices(iter+1,  m_Edges.end_partition(partition), 1);
              }
            }
            m_Edges.delete_from_partition_if(partition, fn);
          }

          if constexpr (!emptyNodes) Nodes::erase_node(this->cbegin_node_weights() + node);
          m_Edges.delete_slot(node);
          fix_edge_data(node,
                      [](const auto targetNode, const auto node) { return targetNode > node; },
                      [](const auto index) { return index - 1; });
        }
        else
        {
          for(size_type i{}; i < m_Edges.num_partitions(); ++i)
          {
            if(i == node) continue;
            m_Edges.delete_from_partition_if(i, [node](const edge_type& e) {
                return (e.target_node() == node);
            });
          }

          if constexpr (!emptyNodes) Nodes::erase_node(this->cbegin_node_weights() + node);
          m_Edges.delete_slot(node);
          fix_edge_data(node,
                      [](const auto targetNode, const auto node) { return targetNode > node; },
                      [](const auto index) { return index - 1; });
        }
      }

      template<class Fn>
      constexpr void mutate_edge_weight(const_edge_iterator citer, Fn fn)
      {
        static_assert(!std::is_empty_v<edge_weight_type>, "Cannot mutate an empty weight!");

        if constexpr (   (!directed(directedness) && !EdgeTraits::shared_weight_v)
                      || (directed(directedness) && EdgeTraits::mutual_info_v && !EdgeTraits::shared_edge_v))
        {
          mutate_partner_edge_weight(citer, fn);
          mutate_host_edge_weight(citer, fn);          
        }
        else
        {
          mutate_host_edge_weight(citer, fn);
        }
      }

      template<class Arg, class... Args>
      constexpr void set_edge_weight(const_edge_iterator citer, Arg&& arg, Args&&... args)
      {
        static_assert(!std::is_empty_v<edge_weight_type>, "Cannot set an empty weight!");

        if constexpr (   (!directed(directedness) && !EdgeTraits::shared_weight_v)
                      || (directed(directedness) && EdgeTraits::mutual_info_v && !EdgeTraits::shared_edge_v))
        {
          auto partnerIter{set_partner_edge_weight(citer, std::forward<Arg>(arg), std::forward<Args>(args)...)};
          set_host_edge_weight(citer, partnerIter->weight());          
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

      
      template<class Fn>
      constexpr void mutate_edge_weight(const_reverse_edge_iterator criter, Fn fn)
      {
        const auto host{criter.partition_index()};
        mutate_edge_weight(m_Edges.cbegin_partition(host) + distance(criter, m_Edges.crend_partition(host)) - 1, fn);
      }

      template<class... Args>
      void join(const edge_index_type node1, const edge_index_type node2, Args&&... args)
      {        
        if constexpr (std::is_empty_v<edge_weight_type>)
          static_assert(sizeof...(args) == 0, "Makes no sense to supply arguments for an empty weight!");
        
        if constexpr (throwOnRangeError) if(node1 >= order() || node2 >= order()) throw std::out_of_range("Graph::join - index out of range");

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
        }
        else
        {
          if constexpr(edge_type::flavour == edge_flavour::partial)
          {
            m_Edges.push_back_to_partition(node1, node2, this->make_edge_weight(std::forward<Args>(args)...));
          }
          else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            const edge_index_type dist(distance(cbegin_edges(node2), cend_edges(node2)));
            const edge_index_type i{node1 == node2 ? dist + 1 : dist};
            m_Edges.push_back_to_partition(node1, node2, i, this->make_edge_weight(std::forward<Args>(args)...));
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            m_Edges.push_back_to_partition(node1, node1, node2, this->make_edge_weight(std::forward<Args>(args)...));
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
        else if constexpr(EdgeTraits::shared_edge_v)
        {
          m_Edges.push_back_to_partition(node2, --cend_edges(node1));
        } 
      }      

      template<class... Args>
      std::pair<const_edge_iterator, const_edge_iterator>
      insert_join(const_edge_iterator citer1, const_edge_iterator citer2, Args&&... args)
      {        
        const auto node1{citer1.partition_index()}, node2{citer2.partition_index()};
        const auto pos2{static_cast<edge_index_type>(distance(cbegin_edges(node2), citer2))};
        if(node1 == node2) return insert_join(citer1, pos2, std::forward<Args>(args)...);

        citer1 = insert_single_join(citer1, node2, pos2, std::forward<Args>(args)...);

        if constexpr(EdgeTraits::shared_edge_v)
        {
          citer2 = m_Edges.insert_to_partition(cbegin_edges(node2) + pos2, citer1);
        }
        else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
        {
          increment_comp_indices(begin_edges(node2) + pos2, end_edges(node2), 1);
          const auto pos1{static_cast<edge_index_type>(distance(cbegin_edges(node1), citer1))};
          citer2 = m_Edges.insert_to_partition(cbegin_edges(node2) + pos2, node1, pos1, *citer1);
        }

        return {citer1, citer2};
      }

      template<class... Args>
      std::pair<const_edge_iterator, const_edge_iterator>
      insert_join(const_edge_iterator citer1, const edge_index_type pos2, Args&&... args)
      {
        const auto node{citer1.partition_index()};
        citer1 = insert_single_join(citer1, node, pos2, std::forward<Args>(args)...);

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
          if(pos2 > pos1)
          {
            increment_comp_indices(begin_edges(node) + pos2, end_edges(node), 1);
          }
          else
          {
            auto iter1{begin_edges(node) + distance(cbegin_edges(node), citer1)};
            increment_comp_indices(begin_edges(node) + pos2, iter1, 1);
            increment_comp_indices(iter1+1, end_edges(node), 1);            
          }

          auto citer2{m_Edges.insert_to_partition(cbegin_edges(node) + pos2, node, pos1, *citer1)};
          citer1 = cbegin_edges(node) + pos1;
          return {citer1, citer2};
        }
      }

      void delete_edge(const_edge_iterator citer)
      {              
        if constexpr (EdgeTraits::mutual_info_v)
        {          
          const auto host{citer.partition_index()};
          const auto other{[&edge=*citer](const auto& host){
              if constexpr (directed(directedness))
              {
                const auto target{edge.target_node()};
                return target == host ? edge.host_node() : target;
              }
              else
              {
                return edge.target_node();
              }
            }(host)};

          constexpr bool embeddedEdge{(edge_type::flavour == edge_flavour::partial_embedded) || (edge_type::flavour == edge_flavour::full_embedded)};
          if(host != other)
          {                        
            if constexpr (embeddedEdge)
            {
              const auto otherLocalIndex{citer->complementary_index()};
              const auto dist{distance(m_Edges.cbegin_partition(host), citer)};
              decrement_comp_indices(m_Edges.begin_partition(host) + dist + 1, m_Edges.end_partition(host), 1);
              m_Edges.delete_from_partition(citer);             
              decrement_comp_indices(m_Edges.begin_partition(other) + otherLocalIndex + 1, m_Edges.end_partition(other), 1);
              m_Edges.delete_from_partition(other, otherLocalIndex);
            }
            else
            {
              const auto otherDist{[this, other](const auto host, const auto citer){
                  if constexpr (directed(directedness))
                  {
                    const auto pEdge{&*citer};
                    for(auto oiter = cbegin_edges(other); oiter != cend_edges(other); ++oiter)
                    {
                      if(&*oiter == pEdge) return distance(cbegin_edges(other), oiter);
                    }
                  }              
                  else
                  {
                    for(auto oiter{cbegin_edges(other)}; oiter != cend_edges(other); ++oiter)
                    {
                      if(oiter->target_node() == host)
                      {
                        if constexpr(std::is_empty_v<edge_weight_type>)
                        {
                          return distance(cbegin_edges(other), oiter);
                        }
                        else
                        {
                          if(citer->weight() == oiter->weight())
                            return distance(cbegin_edges(other), oiter);
                        }
                      }
                    }
                  }

                  throw std::logic_error("Deletion of undirected edge: partner not found");
                }(host, citer)};
                            
              m_Edges.delete_from_partition(citer);              
              m_Edges.delete_from_partition(cbegin_edges(other) + otherDist);
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
                decrement_comp_indices(m_Edges.begin_partition(host) + hostLocalIndex + 1, m_Edges.end_partition(host), 2);
                const auto next{m_Edges.delete_from_partition(citer)};
                m_Edges.delete_from_partition(next);
              }
              else if(dist == -1)
              {
                decrement_comp_indices(m_Edges.begin_partition(host) + hostLocalIndex + 2, m_Edges.end_partition(host), 2);
                const auto next{m_Edges.delete_from_partition(citer-1)};
                m_Edges.delete_from_partition(next);
              }
              else
              {                
                const auto deleteFirstIndex{(hostLocalIndex > dist) ? hostLocalIndex : dist};
                const auto deleteSecondIndex{(hostLocalIndex > dist) ? dist : hostLocalIndex};

                const auto firstIter{m_Edges.begin_partition(host) + deleteFirstIndex};
                const auto secondIter{m_Edges.begin_partition(host) + deleteSecondIndex};

                decrement_comp_indices(firstIter + 1, m_Edges.end_partition(host), 2);                
                decrement_comp_indices(secondIter + 1, firstIter, 1);
                
                m_Edges.delete_from_partition(host, deleteFirstIndex);
                m_Edges.delete_from_partition(host, deleteSecondIndex);
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
                m_Edges.delete_from_partition(citer);
                m_Edges.delete_from_partition(hiter);
              }
              else
              {
                m_Edges.delete_from_partition(hiter);
                m_Edges.delete_from_partition(citer);
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
                  
                    m_Edges.delete_from_partition(ci);
                    break;
                  }
                }
              }              

              m_Edges.delete_from_partition(cbegin_edges(host) + dist);                 
            }           
          }   
        }
        else
        {
          m_Edges.delete_from_partition(citer);
        }
      }

      void clear() noexcept
      {
        if constexpr (!emptyNodes) Nodes::clear();
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
      using bitset = std::vector<bool>;
      static constexpr bool throwOnRangeError{edge_storage_type::throw_on_range_error()};
      static constexpr bool emptyNodes{std::is_empty_v<typename Nodes::weight_type>};
      static constexpr bool protectiveProxy{std::is_same_v<typename Nodes::weight_proxy_type, utilities::protective_wrapper<typename Nodes::weight_type>>};

      edge_storage_type m_Edges;

      static constexpr auto direct_edge_init() noexcept
      {
        return std::bool_constant<std::is_same_v<edge_type, edge_init_type>>{};
      }

      static constexpr auto direct_node_init() noexcept
      {
        return std::bool_constant<emptyNodes || std::is_same_v<typename Nodes::weight_proxy_type, utilities::protective_wrapper<node_weight_type>>>{};
      }

      constexpr void check_consistency(std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
        if constexpr (EdgeTraits::init_complementary_data_v)
        {
          check_consistency_complementary(edges);
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
                m_Edges.push_back_to_partition(host, edge_type{edge.target_node(), WeightMaker::make_edge_weight(edge.weight())});
              }
            }
          }
        }
        else
        {
          check_consistency(direct_edge_init(), edges);          
        }
      }

      constexpr void check_consistency_complementary(std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
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
              auto other{target};
              if constexpr(directed(directedness))
              {
                if(target == currentNodeIndex) other = edge.host_node();
              }
              
              const auto pos{static_cast<edge_index_type>(std::distance(nodeEdges.begin(), edgeIter))};
              if(!EdgeTraits::shared_edge_v || (other > currentNodeIndex) || ((other == currentNodeIndex) && (compIndex > pos)))
              {
                m_Edges.push_back_to_partition(currentNodeIndex, make_edge(currentNodeIndex, edge));
              }
              else
              {
                m_Edges.push_back_to_partition(currentNodeIndex, cbegin_edges(other) + compIndex);
              }
            }
          }
        }
      }

      constexpr void check_consistency(std::false_type, std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
        data_structures::contiguous_storage<edge_init_type, data_sharing::independent<edge_init_type>, true, std::vector> edgesForChecking{edges};
        check_consistency(edgesForChecking);
      }

      constexpr void check_consistency(std::true_type, std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {
        check_consistency(m_Edges);
      }

      template<class Edges>
      constexpr void check_consistency(Edges& orderedEdges)
      {
        constexpr bool sortWeights{!std::is_empty_v<edge_weight_type> && utilities::is_orderable_v<edge_weight_type>};
        constexpr bool clusterEdges{!std::is_empty_v<edge_weight_type> && !utilities::is_orderable_v<edge_weight_type>};
        for(size_type i{}; i< orderedEdges.num_partitions(); ++i)
        {
          sequoia::sort(orderedEdges.begin_partition(i), orderedEdges.end_partition(i), [](const auto& e1, const auto& e2){
            if constexpr (!sortWeights)
            {
              return e1.target_node() < e2.target_node();
            }
            else
            {
              return (e1.target_node() == e2.target_node()) ? e1.weight() < e2.weight() : e1.target_node() < e2.target_node();
            }
          });

          if constexpr(clusterEdges)
          {
            auto lowerIter = orderedEdges.begin_partition(i), upperIter = orderedEdges.begin_partition(i);
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

        // TO DO: use a more efficient search for orderable weights

           
        for(size_type i{}; i<orderedEdges.num_partitions(); ++i)
        {
          if constexpr(!direct_edge_init()) m_Edges.add_slot();
          
          auto lowerIter = orderedEdges.cbegin_partition(i), upperIter = orderedEdges.cbegin_partition(i);
          while(lowerIter != orderedEdges.cend_partition(i))
          {
            auto target = lowerIter->target_node();
            if((target >= orderedEdges.num_partitions()) || (target < edge_index_type{})) throw std::logic_error("Target index out of range");
            {
              auto comparer =[&edge=*lowerIter](const auto uIter){
                if constexpr (std::is_empty_v<edge_weight_type>)
                {
                  return uIter->target_node() == edge.target_node();
                }
                else
                {                   
                  return (uIter->target_node() == edge.target_node()) && (uIter->weight() == edge.weight());
                }
              };

              while((upperIter != orderedEdges.cend_partition(i)) && comparer(upperIter))
                ++upperIter;
            }
               
            const auto count{distance(lowerIter, upperIter)};

            if(target == i)
            {
              if(count % 2) throw std::logic_error("Odd number of loop edges");
              if constexpr(!direct_edge_init())
              {
                for(; lowerIter != upperIter; ++lowerIter)
                {
                  if(!(distance(lowerIter, upperIter) % 2) || ~EdgeTraits::shared_edge_v)
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
              auto comparer = [i](const auto targetIter, const auto& e){
                if constexpr (std::is_empty_v<edge_weight_type>)
                {
                  return targetIter->target_node() != i;
                }
                else
                {
                  return targetIter->target_node() == i ? targetIter->weight() != e.weight() : true;
                }
              };               
               
              auto targetEdgesIter = orderedEdges.cbegin_partition(target);
              while((targetEdgesIter != orderedEdges.cend_partition(target)) && comparer(targetEdgesIter, *lowerIter))
                ++targetEdgesIter;               
               
              auto upperTargetEdgesIter = targetEdgesIter;
              while((upperTargetEdgesIter != orderedEdges.cend_partition(target)) && !comparer(upperTargetEdgesIter, *lowerIter))
                ++upperTargetEdgesIter;

              if(distance(targetEdgesIter, upperTargetEdgesIter) != count)
                throw std::logic_error("Reciprocated target indices do not match");

              if constexpr(!direct_edge_init())
              {                
                for(; lowerIter != upperIter; ++lowerIter)
                {
                  if((i < target) || !EdgeTraits::shared_edge_v)
                  {
                    m_Edges.push_back_to_partition(i, make_edge(i, *lowerIter));
                  }
                  else
                  {
                    const auto compIndex{static_cast<size_t>(distance(orderedEdges.cbegin_partition(target), upperTargetEdgesIter + distance(upperIter, lowerIter)))};
                    m_Edges.push_back_to_partition(i, cbegin_edges(i) + compIndex);
                  }
                }
              }
            }
               
            lowerIter = upperIter;
          }
        }
      }
      
      constexpr graph_primitive(std::true_type, std::true_type, std::initializer_list<std::initializer_list<edge_init_type>> edges, std::initializer_list<node_weight_type> nodeWeights)
        : Nodes{nodeWeights}, m_Edges{edges}
      {
      }

      constexpr graph_primitive(std::true_type, std::false_type, std::initializer_list<std::initializer_list<edge_init_type>> edges, std::initializer_list<node_weight_type> nodeWeights)
        : m_Edges{edges}
      {
        for(const auto& w : nodeWeights)
        {
          Nodes::add_node(WeightMaker::make_node_weight(w));
        }
      }

      constexpr graph_primitive(std::false_type, std::true_type, std::initializer_list<std::initializer_list<edge_init_type>> edges, std::initializer_list<node_weight_type> nodeWeights)
        : Nodes{nodeWeights}
      {
        check_consistency(edges);
      }

      constexpr graph_primitive(std::false_type, std::false_type, std::initializer_list<std::initializer_list<edge_init_type>> edges, std::initializer_list<node_weight_type> nodeWeights)
      {
        check_consistency(edges);
        for(const auto& w : nodeWeights)
        {
          Nodes::add_node(WeightMaker::make_node_weight(w));
        }
      }

      constexpr graph_primitive(std::true_type, std::true_type, std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : Nodes(edges.size())
        , m_Edges{edges}
      {      
        check_consistency(edges);
      }
      
      constexpr graph_primitive(std::true_type, std::false_type, std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : m_Edges{edges}
      {        
        check_consistency(edges);
        
        for(int i{}; i<edges.size(); ++i)
          Nodes::add_node(WeightMaker::make_node_weight(node_weight_type{}));
      }
      
      graph_primitive(std::false_type, std::true_type, std::initializer_list<std::initializer_list<edge_init_type>> edges)
        : Nodes(edges.size())
      {
        check_consistency(edges);
      }

      graph_primitive(std::false_type, std::false_type, std::initializer_list<std::initializer_list<edge_init_type>> edges)
      {        
        check_consistency(edges);
        
        for(int i{}; i<edges.size(); ++i)
            Nodes::add_node(WeightMaker::make_node_weight(node_weight_type{}));        
      }

      template<class EdgeInitializer>
      edge_type make_edge(const edge_index_type host, const EdgeInitializer& edgeInit)
      {
        if constexpr(directed(directedness))
        {
          // Directed edges only have comp indices if the edges aren't shared
          // in which case (edge_init_type == edge_type) and this method is
          // never called.
          using inv_t = inverted_constant<true>;
          if constexpr(std::is_empty_v<edge_weight_type>)
          {
            if((edgeInit.target_node() == host) && edgeInit.inverted())
              return edge_type{host, inv_t{}};
            else
              return edge_type{host, edgeInit.target_node()};
          }
          else
          {
            if((edgeInit.target_node() == edgeInit.host_node()) && edgeInit.inverted())
              return edge_type{host, inv_t{}, WeightMaker::make_edge_weight(edgeInit.weight())};
            else
              return edge_type{host, edgeInit.target_node(), WeightMaker::make_edge_weight(edgeInit.weight())};
          }
        }
        else
        {
          if constexpr(std::is_empty_v<edge_weight_type>)
          {
            return edge_type{edgeInit.target_node()};
          }
          else if constexpr(EdgeTraits::init_complementary_data_v && !EdgeTraits::shared_weight_v)
          {
            return edge_type{edgeInit.target_node(), edgeInit.complementary_index(), WeightMaker::make_edge_weight(edgeInit.weight())};
          }
          else
          {
            return edge_type{edgeInit.target_node(), WeightMaker::make_edge_weight(edgeInit.weight())};
          }
        }
      }
            
      constexpr graph_primitive(std::true_type, const Nodes& in) : Nodes{in}
      {
      }

      constexpr graph_primitive(std::false_type, const Nodes& in)
      {
        for(auto citer=in.cbegin_node_weights(); citer != in.cend_node_weights(); ++citer)
        {
          Nodes::add_node(WeightMaker::make_node_weight(*citer));
        }
      }

      graph_primitive(std::false_type, const graph_primitive& in) : graph_primitive(std::bool_constant<protectiveProxy>{}, static_cast<const Nodes&>(in))
      {
        std::map<const edge_weight_type*, std::pair<edge_index_type, edge_index_type>> weightMap;
        for(size_type i{}; i<in.order(); ++i)
        {
          m_Edges.add_slot();
          for(auto inIter = in.cbegin_edges(i); inIter != in.cend_edges(i); ++inIter)
          {
            auto weightPtr = &inIter->weight();
            auto found = weightMap.find(weightPtr);
            if(found == weightMap.end())
            {
              auto appender = [this, &in, inIter, i, &weightMap](const auto& e){
                m_Edges.push_back_to_partition(i, e);
                const std::pair<edge_index_type, edge_index_type> indices{i, static_cast<edge_index_type>(distance(in.cbegin_edges(i), inIter))};
                weightMap.emplace(&inIter->weight(), indices);
              };
              
              if constexpr(edge_type::flavour == edge_flavour::partial)
              {
                const edge_type e{inIter->target_node(), WeightMaker::make_edge_weight(inIter->weight())};
                appender(e);
              }
              else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
              {
                const edge_type e{inIter->target_node(), inIter->complementary_index(), WeightMaker::make_edge_weight(inIter->weight())};
                appender(e);
              }
              else if constexpr(edge_type::flavour == edge_flavour::full)
              {
                const edge_type e{inIter->host_node(), inIter->target_node(), WeightMaker::make_edge_weight(inIter->weight())};
                appender(e);
              }
            }
            else
            {
              const auto& indices = found->second;
              if constexpr(edge_type::flavour == edge_flavour::partial)
              {                
                m_Edges.push_back_to_partition(i, indices.first, *(cbegin_edges(indices.first)+indices.second));
              }
              else if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
              {
                m_Edges.push_back_to_partition(i, indices.first, indices.second, *(cbegin_edges(indices.first)+indices.second));
              }
              else if constexpr(edge_type::flavour == edge_flavour::full)
              {
                m_Edges.push_back_to_partition(i, cbegin_edges(indices.first)+indices.second);
              }
            }
          }
        }
      }

      constexpr graph_primitive(std::true_type, const graph_primitive& in)
        : graph_primitive(std::bool_constant<protectiveProxy>{}, static_cast<const Nodes&>(in))
      {
        m_Edges = in.m_Edges;
      }

      
      template<class Setter, class... Args>
      constexpr void manipulate_host_edge_weight(const_edge_iterator citer, Setter setter)
      {
        const auto host{citer.partition_index()};
        const auto dist{distance(cbegin_edges(host), citer)};
        const auto iter{m_Edges.begin_partition(host) + dist};
        setter(iter);
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
        const auto target{citer->target_node()};
        if constexpr((edge_type::flavour == edge_flavour::partial_embedded) || (edge_type::flavour == edge_flavour::full_embedded))
        {          
          const auto comp{citer->complementary_index()};
          setter(m_Edges.begin_partition(target) + comp);
          return m_Edges.cbegin_partition(target) + comp;
        }
        else
        {
          const auto host{citer.partition_index()};
          if(host == target)
          {
            for(auto iter{m_Edges.begin_partition(target)}; iter != m_Edges.end_partition(target); ++iter)
            {
              if(distance(m_Edges.begin_partition(target), iter) == distance(cbegin_edges(target), citer))
                continue;
              
              if(*iter == *citer)
              {
                setter(iter);
                return cbegin_edges(target) + distance(m_Edges.begin_partition(target), iter);
              }
            }
          }
          else
          {
            for(auto iter{m_Edges.begin_partition(target)}; iter != m_Edges.end_partition(target); ++iter)
            {
              if((iter->target_node() == host) && (iter->weight() == citer->weight()))
              {
                setter(iter);
                return cbegin_edges(target) + distance(m_Edges.begin_partition(target), iter);
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
            increment_comp_indices(begin_edges(node1) + pos1, end_edges(node1), 1);
            citer1 = m_Edges.insert_to_partition(citer1, node2, pos2);
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            if((node1 == node2) && (pos2 <= pos1))
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, inverted_constant<true>{});
            }
            else
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, node2);
            }
          }
        }
        else
        {
          if constexpr(edge_type::flavour == edge_flavour::partial_embedded)
          {
            increment_comp_indices(begin_edges(node1) + pos1, end_edges(node1), 1);
            citer1 = m_Edges.insert_to_partition(citer1, node2, pos2, this->make_edge_weight(std::forward<Args>(args)...));
          }
          else if constexpr(edge_type::flavour == edge_flavour::full)
          {
            if((node1 == node2) && (pos2 <= pos1))
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, inverted_constant<true>{}, this->make_edge_weight(std::forward<Args>(args)...));
            }
            else
            {
              citer1 = m_Edges.insert_to_partition(citer1, node1, node2, this->make_edge_weight(std::forward<Args>(args)...));
            }
          }
        }

        return citer1;
      }

      template<class Iter, class Fn> constexpr void modify_comp_indices(Iter first, Iter last, Fn fn)
      {
        const auto start{first};
        while(first != last)
        {
          const auto host{first.partition_index()}, target{first->target_node()}, compIndex{first->complementary_index()};
          const auto edgeIter{m_Edges.begin_partition(target) + compIndex};
          if(host != target)
          {
            edgeIter->complementary_index(fn(edgeIter->complementary_index()));
          }
          else
          {            
            const auto startPos{static_cast<edge_index_type>(distance(begin_edges(host), start))};
            if(compIndex >= startPos)
            {
              first->complementary_index(fn(compIndex));             
            }
            else if(edgeIter->complementary_index() >= startPos)
            {
              edgeIter->complementary_index(fn(edgeIter->complementary_index()));
            }
          }
           
          ++first;
        }
      }

      template<class Iter> constexpr void decrement_comp_indices(Iter first, Iter last, const edge_index_type num)
      {
        modify_comp_indices(first, last, [num](const auto compIndex) { return compIndex - num; }); 
      }
      
      template<class Iter> constexpr void increment_comp_indices(Iter first, Iter last, const edge_index_type num)
      {
        modify_comp_indices(first, last, [num](const auto compIndex) { return compIndex + num; }); 
      }

      template<class Pred, class Modifier>
      void fix_edge_data(const edge_index_type node, Pred pred, Modifier modifier)
      {
        std::set<edge_type*> doneEdges;
        for(size_type i{}; i < m_Edges.num_partitions(); ++i)
        {
          for(auto iter = m_Edges.begin_partition(i); iter != m_Edges.end_partition(i); ++iter)
          {
            if(doneEdges.find(&(*iter)) == doneEdges.end())
            {
              const auto target = iter->target_node();
              if(pred(target, node)) iter->target_node(modifier(target));

              if constexpr(EdgeTraits::shared_edge_v)
              {
                const auto host = iter->host_node();
                if(pred(host, node)) iter->host_node(modifier(host));
              }

              doneEdges.insert(&(*iter));
            }
          }
        }
      }
    };

    
    template
    <
      graph_flavour GraphFlavour,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnRangeError=true,
      template <class> class NodeWeightStorage=data_sharing::unpooled,
      template <class> class EdgeWeightStorage=data_sharing::unpooled,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy=data_structures::bucketed_storage
    >
    class graph_base : public
      graph_primitive
      <
        to_directedness(GraphFlavour),
        graph_impl::node_storage<typename NodeWeightStorage<NodeWeight>::proxy, std::vector, ThrowOnRangeError>,
        typename graph_impl::edge_traits<GraphFlavour, EdgeWeight, ThrowOnRangeError, EdgeWeightStorage, EdgeStoragePolicy, std::size_t>,
        typename graph_impl::weight_maker<NodeWeightStorage<NodeWeight>, EdgeWeightStorage<EdgeWeight>>
      >
    {
    private:
      using primitive =
        graph_primitive<
          to_directedness(GraphFlavour),
          graph_impl::node_storage<typename NodeWeightStorage<NodeWeight>::proxy, std::vector, ThrowOnRangeError>,
          typename graph_impl::edge_traits<GraphFlavour, EdgeWeight, ThrowOnRangeError, EdgeWeightStorage, EdgeStoragePolicy, std::size_t>,
          typename graph_impl::weight_maker<NodeWeightStorage<NodeWeight>, EdgeWeightStorage<EdgeWeight>>
      >;
    public:
      using graph_primitive<
          to_directedness(GraphFlavour),
          graph_impl::node_storage<typename NodeWeightStorage<NodeWeight>::proxy, std::vector, ThrowOnRangeError>,
          typename graph_impl::edge_traits<GraphFlavour, EdgeWeight, ThrowOnRangeError, EdgeWeightStorage, EdgeStoragePolicy, std::size_t>,
          typename graph_impl::weight_maker<NodeWeightStorage<NodeWeight>, EdgeWeightStorage<EdgeWeight>>
      >::graph_primitive;

      graph_base(const graph_base&)            = default;
      graph_base(graph_base&&)                 = default;
      graph_base& operator=(const graph_base&) = default;
      graph_base& operator=(graph_base&&)      = default;

      using primitive::add_node;
      using primitive::insert_node;
      using primitive::delete_node;

      using primitive::set_edge_weight;
      using primitive::mutate_edge_weight;

      using primitive::join;      
      using primitive::delete_edge;

      using primitive::clear;

      using primitive::reserve_edges;
      using primitive::edges_capacity;
      using primitive::reserve_nodes;
      using primitive::node_capacity;
      using primitive::shrink_to_fit;
      
      constexpr static graph_flavour flavour{GraphFlavour};
    protected:
      ~graph_base() = default;
    };

    template
    <
      directed_flavour Directedness,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnRangeError=true,
      template <class> class NodeWeightStorage=data_sharing::unpooled,
      template <class> class EdgeWeightStorage=data_sharing::unpooled,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy=data_structures::bucketed_storage
    >
    class graph : public
      graph_base
      <
        (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,
        NodeWeight,
        EdgeWeight,
        ThrowOnRangeError,
        NodeWeightStorage,
        EdgeWeightStorage,
        EdgeStoragePolicy
      >
    {
    private:
      using base =
        graph_base<
          (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,
          NodeWeight,
          EdgeWeight,
          ThrowOnRangeError,
          NodeWeightStorage,
          EdgeWeightStorage,
          EdgeStoragePolicy
        >;
    public:
      using graph_base
      <
        (Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected,
        NodeWeight,
        EdgeWeight,
        ThrowOnRangeError,
        NodeWeightStorage,
        EdgeWeightStorage,
        EdgeStoragePolicy
      >::graph_base;

      using base::sort_edges;
    };

    template
    <
      directed_flavour Directedness,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnRangeError=true,
      template <class> class NodeWeightStorage=data_sharing::unpooled,
      template <class> class EdgeWeightStorage=data_sharing::unpooled,
      template <class, class, bool, template<class...> class> class EdgeStoragePolicy=data_structures::bucketed_storage
    >
    class embedded_graph : public
      graph_base
      <
       (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,
        NodeWeight,
        EdgeWeight,
        ThrowOnRangeError,
        NodeWeightStorage,
        EdgeWeightStorage,
        EdgeStoragePolicy
      >
    {
    private:
      static constexpr graph_flavour to_graph_flavour() noexcept
      {
        return (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded;
      }
    public:
      using graph_base
      <
       (Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded,
        NodeWeight,
        EdgeWeight,
        ThrowOnRangeError,
        NodeWeightStorage,
        EdgeWeightStorage,
        EdgeStoragePolicy
      >::graph_base;
      
      using graph_primitive
      <
        Directedness,
        graph_impl::node_storage<typename NodeWeightStorage<NodeWeight>::proxy, std::vector, ThrowOnRangeError>,
        typename graph_impl::edge_traits<to_graph_flavour(), EdgeWeight, ThrowOnRangeError, EdgeWeightStorage, EdgeStoragePolicy, std::size_t>,
        typename graph_impl::weight_maker<NodeWeightStorage<NodeWeight>, EdgeWeightStorage<EdgeWeight>>
      >::insert_join;
    };

    
    template
    <
      directed_flavour Directedness,
      std::size_t Order,
      std::size_t Size,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnRangeError = true,
      class EdgeIndexType = std::size_t
    >
    class static_graph : public
      graph_primitive<
        Directedness,
        graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order, ThrowOnRangeError>,
        typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, ThrowOnRangeError, std::make_unsigned_t<EdgeIndexType>>, 
        typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >
    {
    private:
      using primitive =
        graph_primitive<
          Directedness,
          graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order, ThrowOnRangeError>,
          typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected, Order, Size, EdgeWeight, ThrowOnRangeError, std::make_unsigned_t<EdgeIndexType>>, 
          typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
        >;
      
    public:
      constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed : graph_flavour::undirected};
      
      using graph_primitive<
          Directedness,
          graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order, ThrowOnRangeError>,
          typename graph_impl::static_edge_traits<flavour, Order, Size, EdgeWeight, ThrowOnRangeError, std::make_unsigned_t<EdgeIndexType>>, 
          typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >::graph_primitive;

      using primitive::set_edge_weight;
      using primitive::mutate_edge_weight;
      using primitive::sort_edges;      
    };

    template
    <
      directed_flavour Directedness,
      std::size_t Order,
      std::size_t Size,
      class NodeWeight,
      class EdgeWeight,
      bool ThrowOnRangeError = true,
      class EdgeIndexType = std::size_t
    >
    class static_embedded_graph : public
      graph_primitive<
        Directedness,
        graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order, ThrowOnRangeError>,
        typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, ThrowOnRangeError, std::make_unsigned_t<EdgeIndexType>>, 
        typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >
    {
    private:
      using primitive =
        graph_primitive<
          Directedness,
          graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order, ThrowOnRangeError>,
          typename graph_impl::static_edge_traits<(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded, Order, Size, EdgeWeight, ThrowOnRangeError, std::make_unsigned_t<EdgeIndexType>>, 
          typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
        >;
      
    public:
      constexpr static graph_flavour flavour{(Directedness == directed_flavour::directed) ? graph_flavour::directed_embedded : graph_flavour::undirected_embedded};
      
      using graph_primitive<
          Directedness,
          graph_impl::static_node_storage<utilities::protective_wrapper<NodeWeight>, Order, ThrowOnRangeError>,
          typename graph_impl::static_edge_traits<flavour, Order, Size, EdgeWeight, ThrowOnRangeError, std::make_unsigned_t<EdgeIndexType>>, 
          typename graph_impl::weight_maker<data_sharing::unpooled<NodeWeight>, data_sharing::unpooled<EdgeWeight>>
      >::graph_primitive;

      using primitive::set_edge_weight;
      using primitive::mutate_edge_weight;
    };
  }
}
