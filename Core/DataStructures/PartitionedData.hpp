////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file PartitionedData.hpp
    \brief Classes implementing the concept of a sequence of data which is divided into partitions.
 */

#include "PartitionedDataDetails.hpp"
#include "TypeTraits.hpp"
#include "ArrayUtilities.hpp"
#include "Iterator.hpp"
#include "Algorithms.hpp"
#include "MonotonicSequence.hpp"

#include <string>
#include <numeric>

namespace sequoia
{
  namespace data_structures
  {
    //===================================A Custom Iterator===================================//
    
    template<class Traits, class SharingPolicy, class IndexType>
    using partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, SharingPolicy, partition_impl::mutable_reference, false>::iterator,
          partition_impl::dereference_policy<SharingPolicy, partition_impl::mutable_reference, partition_impl::partition_index_policy<false, IndexType>>
        >;

    template<class Traits, class SharingPolicy, class IndexType>
    using const_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, SharingPolicy, partition_impl::const_reference, false>::iterator,
        partition_impl::dereference_policy<SharingPolicy, partition_impl::const_reference, partition_impl::partition_index_policy<false, IndexType>>
      >;

    template<class Traits, class SharingPolicy, class IndexType>
    using reverse_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, SharingPolicy, partition_impl::mutable_reference, true>::iterator,
        partition_impl::dereference_policy<SharingPolicy, partition_impl::mutable_reference, partition_impl::partition_index_policy<true, IndexType>>
      >;

    template<class Traits, class SharingPolicy, class IndexType>
    using const_reverse_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, SharingPolicy, partition_impl::const_reference, true>::iterator,
          partition_impl::dereference_policy<SharingPolicy, partition_impl::const_reference, partition_impl::partition_index_policy<true, IndexType>>
      >;
        
    //===================================Storage using buckets===================================//

    template<class T, class SharingPolicy> struct bucketed_storage_traits
    {
      constexpr static bool throw_on_range_error{true};

      template<class S> using buckets_type   = std::vector<S, std::allocator<S>>; 
      template<class S> using container_type = std::vector<S, std::allocator<S>>; 
    };
    
    template<class T, class SharingPolicy=data_sharing::independent<T>, class Traits=bucketed_storage_traits<T, SharingPolicy>>
    class bucketed_storage
    {
    private:
      template<class S> using buckets_template   = typename Traits::template buckets_type<S>;
      template<class S> using container_template = typename Traits::template container_type<S>;
        
      using held_type    = typename SharingPolicy::handle_type;
      using bucket_type  = container_template<held_type>;
      using storage_type = buckets_template<bucket_type>;
    public:
      using value_type               = T;
      using size_type                = typename bucket_type::size_type;
      using index_type                = size_type;
      using allocator_type            = typename bucket_type::allocator_type;
      using partitions_allocator_type = typename storage_type::allocator_type;
      using sharing_policy_type       = SharingPolicy;

      using partition_iterator               = partition_iterator<Traits, SharingPolicy, size_type>;
      using const_partition_iterator         = const_partition_iterator<Traits, SharingPolicy, size_type>;
      using reverse_partition_iterator       = reverse_partition_iterator<Traits, SharingPolicy, size_type>;
      using const_reverse_partition_iterator = const_reverse_partition_iterator<Traits, SharingPolicy, size_type>;

      constexpr static bool throw_on_range_error{Traits::throw_on_range_error};
      
      bucketed_storage() noexcept(noexcept(partitions_allocator_type{})) = default;

      explicit bucketed_storage(const partitions_allocator_type& partitionsAllocator) noexcept
        : m_Buckets(partitionsAllocator) {}

      bucketed_storage(std::initializer_list<std::initializer_list<T>> list,
                       const partitions_allocator_type& partitionsAllocator = partitions_allocator_type{},
                       const allocator_type& allocator = allocator_type{})
        : m_Buckets(partitionsAllocator)
      {
        m_Buckets.reserve(list.size());
        for(auto iter{list.begin()}; iter != list.end(); ++iter)
        { 
          add_slot(allocator);
          const auto dist{std::distance(list.begin(), iter)};          
          m_Buckets[dist].reserve(iter->size());
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
      }

      bucketed_storage(const bucketed_storage& in)
        : bucketed_storage(partition_impl::copy_constant<directCopy>{}, in)
      {}

      bucketed_storage(const bucketed_storage& in, const partitions_allocator_type& partitionAllocator, const allocator_type& allocator)
        : m_Buckets(partitionAllocator)
      {
        m_Buckets.reserve(in.m_Buckets.size());

        partition_impl::data_duplicator<SharingPolicy> duplicator;
        for(auto i{in.m_Buckets.cbegin()}; i != in.m_Buckets.cend(); ++i)
        {
          const auto& bucket{*i};
          add_slot(allocator);
          m_Buckets.back().reserve(in.m_Buckets[std::distance(in.m_Buckets.cbegin(), i)].size());
          for(const auto& elt : bucket)
          {
            m_Buckets.back().push_back(duplicator.duplicate(elt));
          }
        }
      }

      bucketed_storage(bucketed_storage&& in) noexcept = default;

      bucketed_storage(bucketed_storage&& in, const partitions_allocator_type& partitionsAllocator, const allocator_type& allocator)
        : m_Buckets(partitionsAllocator)
      {
        m_Buckets.reserve(in.m_Buckets.size());
        
        for(auto&& bucket : in.m_Buckets)
        {
          m_Buckets.push_back(std::vector<held_type, allocator_type>(std::move(bucket), allocator));          
        }
      }

      bucketed_storage& operator=(bucketed_storage&&) = default;
      
      bucketed_storage& operator=(const bucketed_storage& in)
      {
        if(&in == this) return *this;

        if constexpr(directCopy && partition_impl::propagates_on_copy_assignment_v<partitions_allocator_type, allocator_type>)
        {
          m_Buckets = in.m_Buckets;
        }
        else
        {        
          using partition_copy_propagation_t
            = typename std::allocator_traits<partitions_allocator_type>::propagate_on_container_copy_assignment;

          using copy_propagation_t
            = typename std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment;

          using partition_move_propagation_t
            = typename std::allocator_traits<partitions_allocator_type>::propagate_on_container_move_assignment;

          using move_propagation_t
            = typename std::allocator_traits<allocator_type>::propagate_on_container_move_assignment;

          constexpr bool copyConsistentWithMove{
                 (partition_copy_propagation_t::value == partition_move_propagation_t::value)
              && (copy_propagation_t::value == move_propagation_t::value)};

          auto getPartitionAlloc{
            [](const bucketed_storage& current, const bucketed_storage& in){           
              if constexpr(partition_copy_propagation_t::value)
              {
                return in.m_Buckets.get_allocator();
              }
              else
              {
                return current.m_Buckets.get_allocator();
              }
            }
          };

          auto getAlloc{
            [](const bucketed_storage& current, const bucketed_storage& in){
              if constexpr(copy_propagation_t::value)
              {
                if(!in.m_Buckets.empty())
                {
                  return in.m_Buckets.front().get_allocator();
                }
              }
              else
              {
                if(!current.m_Buckets.empty())
                {
                  return current.m_Buckets.front().get_allocator();
                }
              }

              return allocator_type{};
            }
          };
        
          bucketed_storage tmp{in, getPartitionAlloc(*this, in), getAlloc(*this, in)};               
          if constexpr (copyConsistentWithMove)
          {
            *this = std::move(tmp);
          }
          else
          {
            if constexpr (partition_copy_propagation_t::value)
              *this = bucketed_storage(in.m_Buckets.get_allocator());
                           
            tmp.swap(*this);
          }
        }
        
        return *this;
      }

      void swap(bucketed_storage& other) noexcept(noexcept(std::swap(m_Buckets, other.m_Buckets)))
      {
        std::swap(m_Buckets, other.m_Buckets);
      }

      [[nodiscard]]
      size_type size() const
      {
        return std::accumulate(std::cbegin(m_Buckets), std::cend(m_Buckets), size_type{},
          [](const size_type& current, const auto& bucket){
            return current + bucket.size();
        });
      }
      
      [[nodiscard]]
      size_type num_partitions() const noexcept { return m_Buckets.size(); }
      
      void swap_partitions(const size_type i, const size_type j)
      {
        if((i < num_partitions()) && (j < num_partitions()))
        {
          using std::swap;
          swap(m_Buckets[i], m_Buckets[j]);
        }
        else if constexpr(throw_on_range_error)
        {
          throw std::out_of_range("bucketed_storage::swap_partitions - index out of range");
        }        
      }

      void add_slot()
      {
        if(!std::allocator_traits<allocator_type>::is_always_equal::value && !m_Buckets.empty())
        {
          m_Buckets.emplace_back(m_Buckets.back().get_allocator());
        }
        else
        {
          m_Buckets.emplace_back();
        } 
      }

      void add_slot(const allocator_type& allocator)
      {
        if(   !std::allocator_traits<allocator_type>::is_always_equal::value
           && !m_Buckets.empty()
           && (allocator != m_Buckets.back().get_allocator()))
        {
          throw std::logic_error("Unable to introduce a second, distinct element allocator");
        }
        
        m_Buckets.emplace_back(allocator);
      }

      void insert_slot(const size_type pos, const allocator_type& allocator = allocator_type{})
      {
        if(pos < num_partitions())
        {
          auto iter{m_Buckets.begin() + pos};
          m_Buckets.insert(iter, std::vector<held_type, allocator_type>(allocator));
        }
        else
        {
          add_slot(allocator);
        }
      }

      void erase_slot(const size_type n)
      {
        if(n < m_Buckets.size())
        {
          m_Buckets.erase(m_Buckets.begin() + n);
        }
      }

      void reserve_partition(const size_type partition, const size_type size)
      {
        if constexpr(throw_on_range_error) check_range(partition);

        m_Buckets[partition].reserve(size);
      }

      [[nodiscard]]
      size_type partition_capacity(const size_type partition) const
      {
        if constexpr(throw_on_range_error) check_range(partition);

        return m_Buckets[partition].capacity();
      }

      void reserve_partitions(const size_type numPartitions)
      {
        m_Buckets.reserve(numPartitions);
      }

      [[nodiscard]]
      size_type num_partitions_capacity() const noexcept
      {

        return m_Buckets.capacity();
      }

      void shrink_num_partitions_to_fit()
      {
        m_Buckets.shrink_to_fit();
      }

      void shrink_to_fit(const size_type partition)
      {
        if constexpr(throw_on_range_error) check_range(partition);

        m_Buckets[partition].shrink_to_fit();
      }
      
      void shrink_to_fit()
      {
        shrink_num_partitions_to_fit();
        for(auto& b : m_Buckets) b.shrink_to_fit();
      }

      void clear() noexcept
      {
        for(auto& bucket : m_Buckets) bucket.clear();
      }
      
      template<class... Args>
      void push_back_to_partition(const size_type index, Args&&... args)
      {
        if constexpr(throw_on_range_error) check_range(index);

        m_Buckets[index].push_back(SharingPolicy::make(std::forward<Args>(args)...));
      }

      void push_back_to_partition(const size_type index, const_partition_iterator iter)
      {
        if constexpr(throw_on_range_error) check_range(index);

        m_Buckets[index].push_back(*(iter.base_iterator()));
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        if constexpr(throw_on_range_error)
        {
          if(!m_Buckets.size()) throw std::out_of_range("bucketed_storage: no partitions into which to insert");
        }
        
        const auto host{pos.partition_index()};        
        auto iter{m_Buckets[host].insert(pos.base_iterator(), SharingPolicy::make(std::forward<Args>(args)...))};

        return partition_iterator{iter, host};
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, const_partition_iterator setFromIter)
      {
        if constexpr(throw_on_range_error)
        {
          if(!m_Buckets.size()) throw std::out_of_range("bucketed_storage: no partitions into which to insert");
        }
        
        const auto host{pos.partition_index()};     
        auto iter{m_Buckets[host].insert(pos.base_iterator(), *(setFromIter.base_iterator()))};

        return partition_iterator{iter, host};
      }

      partition_iterator erase_from_partition(const size_type index, const size_type pos)
      {
        auto iter{begin_partition(index)};
        auto next{end_partition(index).base_iterator()};
        if(iter != end_partition(index) && pos < static_cast<size_type>(distance(iter, end_partition(index))))
        {
          next = m_Buckets[index].erase((iter + pos).base_iterator());
        }

        return partition_iterator{next, index};
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto partition{iter.partition_index()};
        const auto next{m_Buckets[partition].erase(iter.base_iterator())};
        return partition_iterator{next, partition};
      }

      partition_iterator begin_partition(const size_type i)
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? partition_iterator(m_Buckets[i].begin(), i) : partition_iterator(m_Buckets.back().end(), npos);
      }

      partition_iterator end_partition(const size_type i)
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? partition_iterator(m_Buckets[i].end(), i) : partition_iterator(m_Buckets.back().end(), npos);
      }

      const_partition_iterator begin_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_partition_iterator(m_Buckets[i].cbegin(), i) : const_partition_iterator(m_Buckets.back().cend(), npos);
      }

      const_partition_iterator end_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_partition_iterator(m_Buckets[i].cend(), i) : const_partition_iterator(m_Buckets.back().cend(), npos);
      }

      reverse_partition_iterator rbegin_partition(const size_type i)
      {
        if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? reverse_partition_iterator(m_Buckets[i].rbegin(), i) : reverse_partition_iterator(m_Buckets.front().rend(), npos);
      }

      reverse_partition_iterator rend_partition(const size_type i)
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? reverse_partition_iterator(m_Buckets[i].rend(), i) : reverse_partition_iterator(m_Buckets.front().rend(), npos);
      }

      const_reverse_partition_iterator rbegin_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator(m_Buckets[i].crbegin(), i) : const_reverse_partition_iterator(m_Buckets.front().crend(), npos);

      }

      const_reverse_partition_iterator rend_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator(m_Buckets[i].crend(), i) : const_reverse_partition_iterator(m_Buckets.front().crend(), npos);
      }

      const_partition_iterator cbegin_partition(const size_type i) const
      {
        return begin_partition(i);
      }

      const_partition_iterator cend_partition(const size_type i) const
      {
        return end_partition(i);
      }

      const_reverse_partition_iterator crbegin_partition(const size_type i) const
      {
        return rbegin_partition(i);
      }

      const_reverse_partition_iterator crend_partition(const size_type i) const
      {
        return rend_partition(i);
      }

      [[nodiscard]]
      const_partition_iterator operator[](const size_type i) const { return cbegin_partition(i); }

      [[nodiscard]]
      partition_iterator operator[](const size_type i) { return begin_partition(i); }

      [[nodiscard]]
      friend bool operator==(const bucketed_storage& lhs, const bucketed_storage& rhs) noexcept
      {
        if constexpr(std::is_same_v<SharingPolicy, data_sharing::independent<T>>)
        {
          return lhs.m_Buckets == rhs.m_Buckets;
        }
        else
        {
          return isomorphic(lhs, rhs);
        }        
      }

      [[nodiscard]]
      friend bool operator!=(const bucketed_storage& lhs, const bucketed_storage& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    private:
      constexpr static auto npos{partition_iterator::npos};
      constexpr static bool directCopy{partition_impl::direct_copy_v<SharingPolicy, T>};

      storage_type m_Buckets;

      bucketed_storage(partition_impl::direct_copy_type, const bucketed_storage& in)
        : m_Buckets{in.m_Buckets}
      {}

      bucketed_storage(partition_impl::indirect_copy_type, const bucketed_storage& in)
        : bucketed_storage(in, in.m_Buckets.get_allocator(), get_allocator(in))
      {}

      allocator_type get_allocator(const bucketed_storage& in)
      {
        return !in.m_Buckets.empty() ? in.m_Buckets.front().get_allocator() :  allocator_type{};
      }
      
      void check_range(const size_type index) const
      {
        if(index >= m_Buckets.size())
        {
          throw std::out_of_range("bucketed_storage::index " + std::to_string(index) + " out of range!");
        }
      }

      void check_range(const size_type index, const size_type pos) const
      {
        check_range(index);
        const auto bucketSize{m_Buckets[index].size()};
        if(pos > bucketSize)
        {
          throw std::out_of_range("bucketed_storage::partition " + std::to_string(index) + " pos " + std::to_string(pos) + " out of range");
        }
      }
    };

    //===================================Contiguous storage===================================//

    template<class T, class SharingPolicy, class Traits>
    class contiguous_storage_base
    {
    public:
      using value_type          = T;
      using sharing_policy_type = SharingPolicy;
      using traits_type         = Traits;
      
      using container_type      = typename partition_impl::storage_type_generator<Traits, SharingPolicy>::container_type;
      using size_type           = std::common_type_t<typename Traits::partition_index_type, typename container_type::size_type>;
      using index_type          = typename Traits::index_type;

      using partition_iterator               = partition_iterator<Traits, SharingPolicy, index_type>;
      using const_partition_iterator         = const_partition_iterator<Traits, SharingPolicy, index_type>;
      using reverse_partition_iterator       = reverse_partition_iterator<Traits, SharingPolicy, index_type>;
      using const_reverse_partition_iterator = const_reverse_partition_iterator<Traits, SharingPolicy, index_type>;

      constexpr static bool throw_on_range_error{Traits::throw_on_range_error};
      
      constexpr contiguous_storage_base() = default;

      constexpr contiguous_storage_base(std::initializer_list<std::initializer_list<T>> list)
        : contiguous_storage_base(init_constant<staticStorage>{}, list)
      {}

      constexpr contiguous_storage_base(const contiguous_storage_base& in)
        : contiguous_storage_base(partition_impl::copy_constant<directCopy>{}, in)
      {}

      constexpr contiguous_storage_base(contiguous_storage_base&& in) noexcept = default;

      constexpr contiguous_storage_base& operator=(contiguous_storage_base&&) noexcept = default;
      
      constexpr contiguous_storage_base& operator=(const contiguous_storage_base& in)
      {
        contiguous_storage_base tmp{in};
        *this = std::move(tmp);
        return *this;
      }

      [[nodiscard]]
      constexpr auto size() const noexcept { return m_Storage.size(); }

      [[nodiscard]]
      constexpr auto num_partitions() const noexcept { return m_Partitions.size(); }

      [[nodiscard]]
      constexpr partition_iterator begin_partition(const index_type i) noexcept
      {
        return get_begin_iterator<partition_iterator>(i, m_Storage.begin());
      }

      [[nodiscard]]
      constexpr partition_iterator end_partition(const index_type i) noexcept
      {
        return get_end_iterator<partition_iterator>(i, m_Storage.begin());
      }

      [[nodiscard]]
      constexpr const_partition_iterator begin_partition(const index_type i) const noexcept
      {
        return get_begin_iterator<const_partition_iterator>(i, m_Storage.cbegin());
      }

      [[nodiscard]]
      constexpr const_partition_iterator end_partition(const index_type i) const noexcept
      {
        return get_end_iterator<const_partition_iterator>(i, m_Storage.cbegin());
      }

      [[nodiscard]]
      constexpr const_partition_iterator cbegin_partition(const index_type i) const noexcept
      {
        return begin_partition(i);
      }      

      [[nodiscard]]
      constexpr const_partition_iterator cend_partition(const index_type i) const noexcept
      {
        return end_partition(i);
      }

      [[nodiscard]]
      constexpr reverse_partition_iterator rbegin_partition(const index_type i) noexcept
      {
        return get_end_iterator<reverse_partition_iterator>(i, m_Storage.rend());
      }

      [[nodiscard]]
      constexpr reverse_partition_iterator rend_partition(const index_type i) noexcept
      {
        return get_begin_iterator<reverse_partition_iterator>(i, m_Storage.rend());
      }

      [[nodiscard]]
      constexpr const_reverse_partition_iterator rbegin_partition(const index_type i) const noexcept
      {
        return get_end_iterator<const_reverse_partition_iterator>(i, m_Storage.crend());
      }

      [[nodiscard]]
      constexpr const_reverse_partition_iterator rend_partition(const index_type i) const noexcept
      {
        return get_begin_iterator<const_reverse_partition_iterator>(i, m_Storage.crend());
      }

      [[nodiscard]]
      constexpr const_reverse_partition_iterator crbegin_partition(const index_type i) const noexcept
      {
        return rbegin_partition(i);
      }

      [[nodiscard]]
      constexpr const_reverse_partition_iterator crend_partition(const index_type i) const noexcept
      {
        return rend_partition(i);
      }

      [[nodiscard]]
      constexpr const_partition_iterator operator[](const index_type i) const noexcept { return cbegin_partition(i); }

      [[nodiscard]]
      constexpr partition_iterator operator[](const index_type i) noexcept { return begin_partition(i); }

      constexpr void swap_partitions(size_type i, size_type j)
      {
        if((i < num_partitions()) && (j < num_partitions()))
        {
          if(i != j)
          {           
            if(i > j) sequoia::swap(i, j);

            const auto len_i{distance(begin_partition(i), end_partition(i))};
            const auto len_j{distance(begin_partition(j), end_partition(j))};

            auto begin_i{begin_partition(i).base_iterator()}, begin_j{begin_partition(j).base_iterator()};
            auto end_i{end_partition(i).base_iterator()}, end_j{end_partition(j).base_iterator()};
            for(auto iter_i{begin_i}, iter_j{begin_j}; (iter_i != end_i) && (iter_j != end_j); ++iter_i, ++iter_j)
            {
              sequoia::iter_swap(iter_i, iter_j);
            }

            if(len_i > len_j)
            {
              sequoia::rotate(begin_i + len_j, begin_i + len_i, end_j);
            }
            else if(len_j > len_i)
            {
              sequoia::rotate(begin_i + len_i, begin_j + len_i, end_j);
            }

            if(len_i != len_j)
            {
              m_Partitions.template mutate<false>(m_Partitions.begin() + i, m_Partitions.begin() + j,
                                  [len_i, len_j](const auto index){ return index + len_j - len_i;});
            }     
          }
        }
        else if constexpr(throw_on_range_error)
        {
          throw std::out_of_range("contiguous_storage::swap_partitions - index out of range");
        }        
      }

      [[nodiscard]]
      friend constexpr bool operator==(const contiguous_storage_base& lhs, const contiguous_storage_base& rhs) noexcept
      {
        if constexpr(std::is_same_v<SharingPolicy, data_sharing::independent<T>>)
        {
          return (lhs.m_Storage == rhs.m_Storage) && (lhs.m_Partitions == rhs.m_Partitions);
        }
        else
        {
          return isomorphic(lhs, rhs);
        }
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const contiguous_storage_base& lhs, const contiguous_storage_base& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    protected:
      template<class Allocator>
      constexpr explicit contiguous_storage_base(const Allocator& allocator) noexcept
        : m_Storage(allocator)
      {}

      template<class PartitionsAllocator, class Allocator>
      constexpr contiguous_storage_base(const PartitionsAllocator& partitionsAllocator, const Allocator& allocator) noexcept
        : m_Partitions(partitionsAllocator)
        , m_Storage(allocator)
      {}

      template<class PartitionsAllocator, class Allocator>
      constexpr contiguous_storage_base(std::initializer_list<std::initializer_list<T>> list, const PartitionsAllocator& partitionsAllocator, const Allocator& allocator)
        : m_Partitions(partitionsAllocator)
        , m_Storage(allocator)
      {
        init(list);
      }
      
      template<class PartitionsAllocator, class Allocator>
      constexpr contiguous_storage_base(const contiguous_storage_base& in, const PartitionsAllocator& partitionsAllocator, const Allocator& allocator)
        : contiguous_storage_base(partition_impl::copy_constant<directCopy>{}, in, partitionsAllocator, allocator)
      {};
      
      template<class PartitionsAllocator, class Allocator>
      constexpr contiguous_storage_base(contiguous_storage_base&& in, const PartitionsAllocator& partitionsAllocator, const Allocator& allocator)
        : m_Partitions{std::move(in.m_Partitions), partitionsAllocator}, m_Storage{std::move(in.m_Storage), allocator}
      {};

      void add_slot()
      {
        m_Partitions.push_back(m_Storage.size());
      }

      void insert_slot(const size_t pos)
      {
        if(pos < num_partitions())
        {
          auto iter{m_Partitions.begin() + pos};
          const index_type newPartitionBound{(pos == 0) ? 0 : *(iter - 1)};
          m_Partitions.insert(iter, newPartitionBound);
        }
        else
        {
          add_slot();
        }
      }

      void erase_slot(const index_type n)
      {
        if(n < m_Partitions.size())
        {
          index_type erased{};
          const index_type offset{(n > 0) ? m_Partitions[n - 1] : index_type{}};
          if(m_Partitions[n] > 0)
          {
            erased = m_Partitions[n] - offset;
            m_Storage.erase(m_Storage.begin() + offset, m_Storage.begin() + m_Partitions[n]);
          }
          m_Partitions.erase(m_Partitions.begin() + n);
          m_Partitions.template mutate<false>(m_Partitions.begin() + n, m_Partitions.end(), [erased](const auto index){ return index - erased; });
        }
      }

      void reserve(const size_type size)
      {
        m_Storage.reserve(size);
      }

      index_type capacity() const noexcept
      {
        return m_Storage.capacity();
      }

      void reserve_partitions(const size_type numPartitions)
      {
        m_Partitions.reserve(numPartitions);
      }

      index_type num_partitions_capacity() const noexcept
      {
        return m_Partitions.capacity();
      }

      void shrink_to_fit()
      {
        m_Partitions.shrink_to_fit();
        m_Storage.shrink_to_fit();
      }

      void clear() noexcept
      {
        m_Partitions.clear();
        m_Storage.clear();
      }
      
      template<class... Args>
      void push_back_to_partition(const index_type index, Args&&... args)
      {
        if constexpr(throw_on_range_error) check_range(index);
        auto maker{
          [](auto&&... a) {
            return SharingPolicy::make(std::forward<decltype(a)>(a)...);
          }
        };
        
        insert(index, maker, std::forward<Args>(args)...);
      }

      void push_back_to_partition(const index_type index, const_partition_iterator iter)
      {
        if constexpr(throw_on_range_error) check_range(index);

         auto maker{
          [](const_partition_iterator i) {
            return *(i.base_iterator());
          }
        };

        insert(index, maker, iter);
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        auto maker{
          [](auto&&... a) {
            return SharingPolicy::make(std::forward<decltype(a)>(a)...);
          }
        };

        return insert(pos, maker, std::forward<Args>(args)...);
      }

      partition_iterator insert_to_partition(const_partition_iterator pos, const_partition_iterator setFromIter)
      {
        auto maker{
          [](const_partition_iterator i) {
            return *(i.base_iterator());
          }
        };

        return insert(pos, maker, setFromIter);
      }

      partition_iterator erase_from_partition(const index_type index, const size_type pos)
      {
        auto next{end_partition(index).base_iterator()};
        auto iter{begin_partition(index)};
        if(iter != end_partition(num_partitions() - 1) && pos < static_cast<size_type>(distance(iter, end_partition(index))))
        {
          next = m_Storage.erase((iter + pos).base_iterator());
          decrement_partition_indices(index);
        }

        return partition_iterator(next, index);
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto next{m_Storage.erase(iter.base_iterator())};
        const auto index{iter.partition_index()};
        decrement_partition_indices(index);
          
        return partition_iterator{next, iter.partition_index()};
      }
    private:
      template<bool Direct>
      struct init_constant : std::bool_constant<Direct>
      {};
      
      using static_init_type  = init_constant<true>;
      using dynamic_init_type = init_constant<false>;      

      constexpr static bool staticStorage{Traits::static_storage_v};
      constexpr static bool directCopy{partition_impl::direct_copy_v<SharingPolicy, T>};

      using PartitionsType = typename Traits::partitions_type;
      constexpr static index_type npos{partition_iterator::npos};

      PartitionsType m_Partitions;
      container_type m_Storage;

      constexpr contiguous_storage_base(static_init_type, std::initializer_list<std::initializer_list<T>> list)
        : m_Partitions{make_partitions(list)}
        , m_Storage{fill(std::make_index_sequence<Traits::num_elements_v>{}, list)}
      {}
      
      contiguous_storage_base(dynamic_init_type, std::initializer_list<std::initializer_list<T>> list)
      {
        init(list);
      }

      constexpr contiguous_storage_base(partition_impl::indirect_copy_type, const contiguous_storage_base& in)
        : m_Partitions{in.m_Partitions}
        , m_Storage(in.m_Storage.get_allocator())
      {
        init(in.m_Storage);
      }

      template<class Allocator>
      constexpr contiguous_storage_base(partition_impl::indirect_copy_type, const contiguous_storage_base& in, const Allocator& allocator)
        : m_Partitions{in.m_Partitions}
        , m_Storage(allocator)        
      {
        init(in.m_Storage);
      }

      template<class Allocator, class PartitionsAllocator>
      constexpr contiguous_storage_base(partition_impl::indirect_copy_type, const contiguous_storage_base& in, const PartitionsAllocator& partitionsAllocator, const Allocator& allocator)
      : m_Partitions{in.m_Partitions, partitionsAllocator}, m_Storage(allocator)        
      {
        init(in.m_Storage);
      }      

      constexpr contiguous_storage_base(partition_impl::direct_copy_type, const contiguous_storage_base& in)
        : m_Partitions{in.m_Partitions}, m_Storage{in.m_Storage}
      {}

      template<class Allocator>
      constexpr contiguous_storage_base(partition_impl::direct_copy_type, const contiguous_storage_base& in, const Allocator& allocator)
        : m_Partitions{in.m_Partitions}, m_Storage{in.m_Storage, allocator}
      {};

      template<class PartitionsAllocator, class Allocator>
      constexpr contiguous_storage_base(partition_impl::direct_copy_type, const contiguous_storage_base& in, const PartitionsAllocator& partitionsAllocator, const Allocator& allocator)
        : m_Partitions{in.m_Partitions, partitionsAllocator}, m_Storage{in.m_Storage, allocator}
      {};

      void init(std::initializer_list<std::initializer_list<T>> list)
      {
        m_Partitions.reserve(list.size());
        m_Storage.reserve(std::accumulate(list.begin(), list.end(), std::size_t{}, [](std::size_t n, auto l) { return n += l.size(); }));
        for(auto iter{list.begin()}; iter != list.end(); ++iter)
        {
          add_slot();
          const auto dist{std::distance(list.begin(), iter)};
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
      }

      void init(const container_type& c)
      {
        m_Storage.reserve(c.size());
        partition_impl::data_duplicator<SharingPolicy> duplicator;
        for(const auto& elt : c)
        {
          m_Storage.push_back(duplicator.duplicate(elt));
        }
      }

      constexpr auto make_element(size_type i, std::initializer_list<std::initializer_list<T>> list, index_type& partition)
      {
        auto boundary{m_Partitions[partition]};
        while(i == boundary)
        {
          boundary = m_Partitions[++partition];
        }

        auto index{partition ? i - m_Partitions[partition-1] : i};
        
        return SharingPolicy::make(*((list.begin() + partition)->begin() + index));
      }

      template<size_type... Inds>
      constexpr container_type fill(std::index_sequence<Inds...>, std::initializer_list<std::initializer_list<T>> list)
      {
        if(list.size() != Traits::num_partitions_v)
          throw std::logic_error("Overall initializer list of wrong size");

        size_type total{};
        for(auto l : list) total += l.size();

        if(total != Traits::num_elements_v)
          throw std::logic_error("Inconsistent number of elements supplied by initializer lists");
        
        if constexpr(sizeof...(Inds) > 0)
        {
          index_type partition{};
          return container_type{ make_element(Inds, list, partition)... };
        }
        else
        {
          return container_type{};
        }
      }

      constexpr static auto make_partitions(std::initializer_list<std::initializer_list<T>> list)
      {
        constexpr auto N{Traits::num_partitions_v};
        auto sizes{utilities::to_array<index_type, N>(list, [](const auto& l){ return l.size(); })};
        if(!sizes.empty())
        {
          for(auto iter{sizes.begin()+1}; iter!=sizes.end(); ++iter)
          {
            *iter += *(iter - 1);
          }
        }

        return sizes;
      }

      void increment_partition_indices(const size_type first) noexcept
      {
        m_Partitions.template mutate<false>(m_Partitions.begin() + first, m_Partitions.end(), [](const auto index){ return index + 1; });
      }

      void decrement_partition_indices(const size_type first) noexcept
      {
        m_Partitions.template mutate<false>(m_Partitions.begin() + first, m_Partitions.end(), [](const auto index){ return index - 1; });
      }

      void check_range(const size_type index) const
      {
        if(index >= m_Partitions.size())
        {
          throw std::out_of_range("contiguous_storage::index" + std::to_string(index) +  "out of range");
        }
      }

      void check_range(const size_type index, const index_type pos) const
      {
        check_range(index);
        const index_type maxPos{index ? m_Partitions[index] - m_Partitions[index - 1] : m_Partitions[index]};
        if(pos > maxPos)
        {
          throw std::out_of_range("contiguous_storage::partition " + std::to_string(index) + " pos " + std::to_string(pos) + " out of range");
        }
      }

      void check_pos_validity(const index_type index, const index_type pos, const index_type expand)
      {
        const index_type maxPos{index ? m_Partitions[index] - m_Partitions[index - 1] : m_Partitions[index]};
        if(pos > maxPos + expand) throw std::out_of_range("contiguous_storage::pos index out of range");
      }

      template<class Maker, class... Args>
      auto insert(const index_type index, Maker maker, Args&&... args)
      {
        auto iter{m_Storage.end()};
        if(index == m_Partitions.size() - 1)
        {
          m_Storage.push_back(maker(std::forward<Args>(args)...));
          iter = --m_Storage.end();
        }
        else
        {
          iter = m_Storage.insert(m_Storage.begin() + m_Partitions[index], maker(std::forward<Args>(args)...));
        }

        increment_partition_indices(index);

        return iter;
      }

      template<class Maker, class... Args>
      auto insert(const_partition_iterator pos, Maker maker, Args&&... args)
      {
        if constexpr(throw_on_range_error)
        {
          if(!m_Partitions.size()) throw std::out_of_range("contiguous_storage: no partitions into which to insert");
        }
         
        const auto host{pos.partition_index()};
        auto iter{m_Storage.insert(pos.base_iterator(), maker(std::forward<Args>(args)...))};
        increment_partition_indices(host);

        return partition_iterator{iter, host};
      }
      
      template<class PartitionIterator, class Iterator>
      constexpr PartitionIterator get_begin_iterator(const index_type i, Iterator iter) const noexcept
      {
        const index_type index{i >= m_Partitions.size() ? npos : i};
        const auto offset{(i > 0 && i < m_Partitions.size()) ? m_Partitions[i-1] : index_type{}};
        return PartitionIterator::reversed() ? PartitionIterator{iter, index} - offset : PartitionIterator{iter, index} + offset;
      }

      template<class PartitionIterator, class Iterator>
      constexpr PartitionIterator get_end_iterator(const index_type i, Iterator iter) const noexcept
      {        
        index_type index{PartitionIterator::reversed() ? index_type{} : npos};
        index_type offset(m_Storage.size());
        if(i < m_Partitions.size())
        {
          index = i;
          offset = m_Partitions[i];
        }

        return PartitionIterator::reversed() ? PartitionIterator{iter, index} - offset : PartitionIterator{iter, index} + offset;
      }
      
    };

    template<class T, class SharingPolicy> struct contiguous_storage_traits
    {
      constexpr static bool static_storage_v{false};
      constexpr static bool throw_on_range_error{true};

      using index_type = std::size_t;
      using partition_index_type = std::size_t;
      using partitions_type = maths::monotonic_sequence<partition_index_type, std::greater<partition_index_type>>;
      using partitions_allocator_type = typename partitions_type::allocator_type;
      
      template<class S> using container_type = std::vector<S, std::allocator<S>>;
    };

    template<class T, class SharingPolicy=data_sharing::independent<T>, class Traits=contiguous_storage_traits<T, SharingPolicy>>
    class contiguous_storage : public contiguous_storage_base<T, SharingPolicy, Traits>
    {
    private:
      using base_t = contiguous_storage_base<T, SharingPolicy, Traits>;
    public:
      using container_type            = typename contiguous_storage_base<T, SharingPolicy, Traits>::container_type;
      using allocator_type            = typename container_type::allocator_type;
      using partitions_allocator_type = typename Traits::partitions_allocator_type;

      contiguous_storage() = default;

      contiguous_storage(const partitions_allocator_type& partitionAllocator, const allocator_type& allocator) noexcept
        : contiguous_storage_base<T, SharingPolicy, Traits>(partitionAllocator, allocator)
      {}

      contiguous_storage(std::initializer_list<std::initializer_list<T>> list, const partitions_allocator_type& partitionAllocator=partitions_allocator_type{}, const allocator_type& allocator=allocator_type{})
        : contiguous_storage_base<T, SharingPolicy, Traits>(list, partitionAllocator, allocator)
      {}

      contiguous_storage(const contiguous_storage&) = default;

      contiguous_storage(const contiguous_storage& s, const partitions_allocator_type& partitionAllocator, const allocator_type& allocator)
        : contiguous_storage_base<T, SharingPolicy, Traits>(s, partitionAllocator, allocator)
      {}
      
      contiguous_storage(contiguous_storage&&) noexcept = default;

      contiguous_storage(contiguous_storage&& s, const partitions_allocator_type& partitionAllocator, const allocator_type& allocator)
        : contiguous_storage_base<T, SharingPolicy, Traits>(std::move(s), partitionAllocator, allocator)
      {}

      ~contiguous_storage() = default;

      contiguous_storage& operator=(const contiguous_storage&)     = default;
      contiguous_storage& operator=(contiguous_storage&&) noexcept = default;

      using base_t::add_slot;
      using base_t::insert_slot;
      using base_t::erase_slot;

      using base_t::reserve;
      using base_t::capacity;
      using base_t::reserve_partitions;
      using base_t::num_partitions_capacity;
      using base_t::shrink_to_fit;
      
      using base_t::clear;
      using base_t::push_back_to_partition;
      using base_t::insert_to_partition;
      using base_t::erase_from_partition;
    };

    template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType> struct static_contiguous_storage_traits
    {
      constexpr static bool static_storage_v{true};
      constexpr static bool throw_on_range_error{true};
      constexpr static std::size_t num_partitions_v{Npartitions};
      constexpr static std::size_t num_elements_v{Nelements};
      
      using index_type = IndexType;
      using partition_index_type = std::make_unsigned_t<IndexType>;      
      using partitions_type = maths::static_monotonic_sequence<partition_index_type, Npartitions, std::greater<partition_index_type>>;
      
      template<class S> using container_type = std::array<S, Nelements>;
    };

    template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType=std::size_t>
    class static_contiguous_storage :
      public contiguous_storage_base<T, data_sharing::independent<T>, static_contiguous_storage_traits<T, Npartitions, Nelements,IndexType>>
    {
    public:
      using contiguous_storage_base<
        T,
        data_sharing::independent<T>,
        static_contiguous_storage_traits<T, Npartitions, Nelements,IndexType>
      >::contiguous_storage_base;
    };
    

    template<class Storage1, class Storage2>
    constexpr bool isomorphic(const Storage1& lhs, const Storage2& rhs) noexcept
    {
      using size_type = std::common_type_t<
        typename Storage1::size_type,
        typename Storage2::size_type
      >;
      
      if(lhs.num_partitions() != rhs.num_partitions()) return false;

      for(size_type i{}; i < lhs.num_partitions(); ++i)
      {
        if(distance(lhs.cbegin_partition(i), lhs.cend_partition(i))
          != distance(rhs.cbegin_partition(i), rhs.cend_partition(i)))
        {
          return false;
        }

        auto iter1{lhs.cbegin_partition(i)};
        auto iter2{rhs.cbegin_partition(i)};
        for(; iter1 != lhs.cend_partition(i); ++iter1, ++iter2)
        {
          if(*iter1 != *iter2) return false;
        }
      }

      return true;
    }    
  }

  template<class PartitionedData, class Pred, class Index=typename PartitionedData::index_type>
  void erase_from_partition_if(PartitionedData& data, const Index partitionIndex, Pred pred)
  {
    auto i{data.begin_partition(partitionIndex)};
    while(i != data.end_partition(partitionIndex))
    {
      i = pred(*i) ? data.erase_from_partition(i) : std::next(i);
    }
  }

  template<class T, class SharingPolicy, class Traits>
  void swap(data_structures::bucketed_storage<T, SharingPolicy, Traits>& lhs, data_structures::bucketed_storage<T, SharingPolicy, Traits>& rhs)
  {
    lhs.swap(rhs);
  }
}
