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

#include <string>
#include <numeric>

namespace sequoia
{
  namespace data_structures
  {
    //===================================A Custom Iterator===================================//
    
    template<template<class...> class C, class SharingPolicy, class IndexType>
    using partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<C, SharingPolicy, partition_impl::mutable_reference, false>::iterator,
          partition_impl::dereference_policy<SharingPolicy, partition_impl::mutable_reference, partition_impl::partition_index_policy<false, IndexType>>
        >;

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using const_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<C, SharingPolicy, partition_impl::const_reference, false>::iterator,
        partition_impl::dereference_policy<SharingPolicy, partition_impl::const_reference, partition_impl::partition_index_policy<false, IndexType>>
      >;

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using reverse_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<C, SharingPolicy, partition_impl::mutable_reference, true>::iterator,
        partition_impl::dereference_policy<SharingPolicy, partition_impl::mutable_reference, partition_impl::partition_index_policy<true, IndexType>>
      >;

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using const_reverse_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<C, SharingPolicy, partition_impl::const_reference, true>::iterator,
          partition_impl::dereference_policy<SharingPolicy, partition_impl::const_reference, partition_impl::partition_index_policy<true, IndexType>>
      >;
        
    //===================================Storage using buckets===================================//

    template<class T, class SharingPolicy> struct bucketed_storage_traits
    {
      constexpr static bool throw_on_range_error{true};

      template<class S> using buckets_type           = std::vector<S, std::allocator<S>>; 
      template<class S> using individual_bucket_type = std::vector<S, std::allocator<S>>; 
    };
    
    template<class T, class SharingPolicy=data_sharing::independent<T>, class Traits=bucketed_storage_traits<T, SharingPolicy>>
    class [[nodiscard]] bucketed_storage
    {
    private:
      template<class S> using buckets_template           = typename Traits::template buckets_type<S>;
      template<class S> using individual_bucket_template = typename Traits::template individual_bucket_type<S>;
        
      using held_type      = typename SharingPolicy::handle_type;
      using bucket_type    = individual_bucket_template<held_type>;
      using storage_type   = buckets_template<bucket_type>;
    public:
      using value_type = T;
      using size_type = typename bucket_type::size_type;
      using sharing_policy_type = SharingPolicy;
      using partition_iterator = partition_iterator<individual_bucket_template, SharingPolicy, size_type>;
      using const_partition_iterator = const_partition_iterator<individual_bucket_template, SharingPolicy, size_type>;
      using reverse_partition_iterator = reverse_partition_iterator<individual_bucket_template, SharingPolicy, size_type>;
      using const_reverse_partition_iterator = const_reverse_partition_iterator<individual_bucket_template, SharingPolicy, size_type>;

      constexpr static bool throw_on_range_error{Traits::throw_on_range_error};
      
      bucketed_storage() noexcept {}

      bucketed_storage(std::initializer_list<std::initializer_list<T>> list)
      {
        for(auto iter = list.begin(); iter != list.end(); ++iter)
        { 
          add_slot();
          const auto dist{std::distance(list.begin(), iter)};
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
      }

      bucketed_storage(const bucketed_storage& in)
      {
        partition_impl::data_duplicator<SharingPolicy> duplicator;
        for(const auto& bucket : in.m_Buckets)
        {
          add_slot();
          for(const auto& elt : bucket)
          {
            m_Buckets.back().push_back(duplicator.duplicate(elt));
          }
        }
      }

      bucketed_storage(bucketed_storage&& in) noexcept = default;

      bucketed_storage& operator=(bucketed_storage&&) noexcept = default;
      
      bucketed_storage& operator=(const bucketed_storage& in)
      {
        bucketed_storage tmp{in};
        std::swap(tmp, *this);
        return *this;
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
          std::swap(m_Buckets[i], m_Buckets[j]);
        }
        else if constexpr(throw_on_range_error)
        {
          throw std::out_of_range("bucketed_storage::swap_partitions - index out of range");
        }        
      }

      void add_slot()
      {
        m_Buckets.push_back(std::vector<held_type>());
      }

      size_type insert_slot(const size_type pos)
      {
        auto partition{pos};
        if(pos < num_partitions())
        {
          auto iter = m_Buckets.begin() + pos;
          m_Buckets.insert(iter, std::vector<held_type>());
        }
        else
        {
          add_slot();
          partition = (num_partitions() - 1);
        }

        return partition;
      }

      size_type erase_slot(const size_type n)
      {
        size_type erased{};
        if(n < m_Buckets.size())
        {
          erased = m_Buckets[n].size();
          m_Buckets.erase(m_Buckets.begin() + n);
        }

        return erased;
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
        auto iter = begin_partition(index);
        auto next = end_partition(index).base_iterator();
        if(iter != end_partition(index) && pos < static_cast<size_type>(distance(iter, end_partition(index))))
        {
          next = m_Buckets[index].erase((iter + pos).base_iterator());
        }

        return partition_iterator{next, index};
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto partition = iter.partition_index();
        const auto next = m_Buckets[partition].erase(iter.base_iterator());
        return partition_iterator{next, partition};
      }

      // TO DO: move to namespace
      template<class UnaryPred>
      size_type erase_from_partition_if(const size_type partitionIndex, UnaryPred pred)
      {
        size_type erased{};
        auto iter = begin_partition(partitionIndex);
        while(iter != end_partition(partitionIndex))
        {
          if(pred(*iter))
          {
            iter = partition_iterator(m_Buckets[partitionIndex].erase(iter.base_iterator()), partitionIndex);
            ++erased;
          }
          else
          {
            ++iter;
          }
        }

        return erased;
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

      storage_type m_Buckets;

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

      partition_iterator insert_directly_to_partition(const size_type index, const size_type pos, const held_type& toAdd)
      {
        if constexpr(throw_on_range_error)
        {
          check_range(index);
          check_range(pos);
        }

        auto bucketIter = m_Buckets[index].cbegin() + pos;
        auto iter = m_Buckets[index].insert(bucketIter, toAdd);

        return partition_iterator(iter, index);
      }
    };

    //===================================Contiguous storage===================================//
    
    template<class T, class SharingPolicy, class Traits>
    class [[nodiscard]] contiguous_storage_base
    {
    private:
      template<class S> using Storage = typename Traits::template underlying_storage_type<S>;
      using AuxiliaryType = typename partition_impl::storage_type_generator<Storage, SharingPolicy>::auxiliary_storage_type;  
      using StorageType   = typename partition_impl::storage_type_generator<Storage, SharingPolicy>::storage_type; 
    public:
      using value_type          = T;
      using sharing_policy_type = SharingPolicy;
      using size_type           = std::common_type_t<typename AuxiliaryType::size_type, typename StorageType::size_type>;
      using index_type          = typename AuxiliaryType::value_type;

      using partition_iterator               = partition_iterator<Storage, SharingPolicy, index_type>;
      using const_partition_iterator         = const_partition_iterator<Storage, SharingPolicy, index_type>;
      using reverse_partition_iterator       = reverse_partition_iterator<Storage, SharingPolicy, index_type>;
      using const_reverse_partition_iterator = const_reverse_partition_iterator<Storage, SharingPolicy, index_type>;

      constexpr static bool throw_on_range_error{Traits::throw_on_range_error};
      
      contiguous_storage_base() noexcept {}

      constexpr contiguous_storage_base(std::initializer_list<std::initializer_list<T>> list) : contiguous_storage_base(StaticType{}, list)
      {}

      constexpr contiguous_storage_base(const contiguous_storage_base& in) : contiguous_storage_base(StaticType{}, in)
      {}
      
      contiguous_storage_base(contiguous_storage_base&& in) noexcept = default;

      contiguous_storage_base& operator=(contiguous_storage_base&&) noexcept = default;
      
      contiguous_storage_base& operator=(const contiguous_storage_base& in)
      {
        contiguous_storage_base tmp{in};
        std::swap(tmp, *this);
        return *this;
      }

      [[nodiscard]]
      constexpr auto size() const noexcept { return m_Storage.size(); }

      [[nodiscard]]
      constexpr auto num_partitions() const noexcept { return m_Partitions.size(); }      
      
      constexpr partition_iterator begin_partition(const index_type i) noexcept
      {
        return get_begin_iterator<partition_iterator>(i, m_Storage.begin());
      }

      constexpr partition_iterator end_partition(const index_type i) noexcept
      {
        return get_end_iterator<partition_iterator>(i, m_Storage.begin());
      }

      constexpr const_partition_iterator begin_partition(const index_type i) const noexcept
      {
        return get_begin_iterator<const_partition_iterator>(i, m_Storage.cbegin());
      }

      constexpr const_partition_iterator end_partition(const index_type i) const noexcept
      {
        return get_end_iterator<const_partition_iterator>(i, m_Storage.cbegin());
      }

      constexpr const_partition_iterator cbegin_partition(const index_type i) const noexcept
      {
        return begin_partition(i);
      }

      constexpr const_partition_iterator cend_partition(const index_type i) const noexcept
      {
        return end_partition(i);
      }
      
      constexpr reverse_partition_iterator rbegin_partition(const index_type i) noexcept
      {
        return get_end_iterator<reverse_partition_iterator>(i, m_Storage.rend());
      }

      constexpr reverse_partition_iterator rend_partition(const index_type i) noexcept
      {
        return get_begin_iterator<reverse_partition_iterator>(i, m_Storage.rend());
      }

      constexpr const_reverse_partition_iterator rbegin_partition(const index_type i) const noexcept
      {
        return get_end_iterator<const_reverse_partition_iterator>(i, m_Storage.crend());
      }

      constexpr const_reverse_partition_iterator rend_partition(const index_type i) const noexcept
      {
        return get_begin_iterator<const_reverse_partition_iterator>(i, m_Storage.crend());
      }

      constexpr const_reverse_partition_iterator crbegin_partition(const index_type i) const noexcept
      {
        return rbegin_partition(i);
      }

      constexpr const_reverse_partition_iterator crend_partition(const index_type i) const noexcept
      {
        return rend_partition(i);
      }

      [[nodiscard]]
      constexpr const_partition_iterator operator[](const index_type i) const noexcept { return cbegin_partition(i); }

      [[nodiscard]]
      partition_iterator operator[](const index_type i) noexcept { return begin_partition(i); }

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
              for(auto iter{m_Partitions.begin() + i}; iter != m_Partitions.begin() + j; ++iter)
              {
                auto& partitionBound{*iter};
                partitionBound = partitionBound + len_j - len_i;
              }
            }     
          }
        }
        else if constexpr(throw_on_range_error)
        {
          throw std::out_of_range("contiguous_storage::swap_partitions - index out of range");
        }        
      }
    protected:
      void add_slot()
      {
        m_Partitions.push_back(m_Storage.size());
      }

      size_type insert_slot(const size_t pos)
      {
        index_type partition{pos};
        if(pos < num_partitions())
        {
          auto iter = m_Partitions.begin() + pos;
          const index_type newPartitionBound = (pos == 0) ? 0 : *(iter - 1);
          m_Partitions.insert(iter, newPartitionBound);
        }
        else
        {
          add_slot();
          partition = (num_partitions() - 1);
        }

        return partition;
      }

      index_type erase_slot(const index_type n)
      {
        index_type erased{};
        if(n < m_Partitions.size())
        {
          const index_type offset{(n > 0) ? m_Partitions[n - 1] : index_type{}};
          if(m_Partitions[n] > 0)
          {
            erased = m_Partitions[n] - offset;
            m_Storage.erase(m_Storage.begin() + offset, m_Storage.begin() + m_Partitions[n]);
          }
          m_Partitions.erase(m_Partitions.begin() + n);
          for(size_type i{n}; i < m_Partitions.size(); ++i)
          {
            m_Partitions[i]-=erased;
          }
        }

        return erased;
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
        insert(index, std::forward<Args>(args)...);
      }

      // Unify this with insert method
      void push_back_to_partition(const index_type index, const_partition_iterator iter)
      {
        if constexpr(throw_on_range_error) check_range(index);
        auto insertIter = m_Storage.end();
        if(index == m_Partitions.size() - 1)
        {
          m_Storage.push_back(*(iter.base_iterator()));
          insertIter = --m_Storage.end();
        }
        else
        {
          insertIter = m_Storage.insert(m_Storage.begin() + m_Partitions[index], *(iter.base_iterator()));
        }

        for(size_type i{index}; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        if constexpr(throw_on_range_error)
        {
          if(!m_Partitions.size()) throw std::out_of_range("contiguous_storage: no partitions into which to insert");
        }
         
        const auto host{pos.partition_index()};
        auto iter{m_Storage.insert(pos.base_iterator(), SharingPolicy::make(std::forward<Args>(args)...))};

        for(auto i{host}; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }

        return partition_iterator(iter, host);
      }

      partition_iterator insert_to_partition(const_partition_iterator pos, const_partition_iterator setFromIter)
      {
        if constexpr(throw_on_range_error)
        {
          if(!m_Partitions.size()) throw std::out_of_range("contiguous_storage: no partitions into which to insert");
        }
        
        const auto host{pos.partition_index()};
        auto iter{m_Storage.insert(pos.base_iterator(), *(setFromIter.base_iterator()))};

        for(auto i{host}; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }

        return partition_iterator(iter, host);
      }

      partition_iterator erase_from_partition(const index_type index, const size_type pos)
      {
        auto next = end_partition(index).base_iterator();
        auto iter = begin_partition(index);
        if(iter != end_partition(num_partitions() - 1) && pos < static_cast<size_type>(distance(iter, end_partition(index))))
        {
          next = m_Storage.erase((iter + pos).base_iterator());
          for(size_type i{index}; i < m_Partitions.size(); ++i)
          {
            m_Partitions[i]-=1;
          }
        }

        return partition_iterator(next, index);
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto next = m_Storage.erase(iter.base_iterator());
        const auto index = iter.partition_index();
        for(size_type i{index}; i < m_Partitions.size(); ++i)
        {
          --m_Partitions[i];
        }
        return partition_iterator{next, iter.partition_index()};
      }

      // TO DO: move to namespace
      template<class UnaryPred>
      index_type erase_from_partition_if(const index_type partitionIndex, UnaryPred pred)
      {
        index_type erased{};
        auto iter = begin_partition(partitionIndex);
        while(iter != end_partition(partitionIndex))
        {
          if(pred(*iter))
          {
            iter = partition_iterator(m_Storage.erase(iter.base_iterator()), partitionIndex);
            ++erased;

            for(size_type i{partitionIndex}; i < m_Partitions.size(); ++i)
            {
              --m_Partitions[i];
            }
          }
          else
          {
            ++iter;
          }
        }

        return erased;
      }

      friend constexpr bool operator==(const contiguous_storage_base& lhs, const contiguous_storage_base& rhs)
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

      friend bool operator!=(const contiguous_storage_base lhs, const contiguous_storage_base& rhs)
      {
        return !(lhs == rhs);
      }

    private:
      using held_type      = typename partition_impl::storage_type_generator<Storage, SharingPolicy>::held_type;
      using container_type = typename partition_impl::storage_type_generator<Storage, SharingPolicy>::container_type;     
      using StaticType     = typename partition_impl::storage_type_generator<Storage, SharingPolicy>::static_type;
      constexpr static index_type npos{partition_iterator::npos};
      
      StorageType m_Storage;
      AuxiliaryType m_Partitions;

      constexpr contiguous_storage_base(std::false_type, const contiguous_storage_base& in) : m_Partitions{in.m_Partitions}
      {
        partition_impl::data_duplicator<SharingPolicy> duplicator;
        for(const auto& elt : in.m_Storage)
        {
          m_Storage.push_back(duplicator.duplicate(elt));
        }
      }

      constexpr contiguous_storage_base(std::true_type, const contiguous_storage_base& in) : m_Storage{in.m_Storage}, m_Partitions{in.m_Partitions}
      {
      }

      template<class PartitionSizes>
      constexpr static held_type make_element(size_type i, std::initializer_list<std::initializer_list<T>> list, const PartitionSizes& partitionSizes)
      {
        index_type partition{}, index{};        
        while(!partitionSizes[partition]) ++partition;
        while(i)
        {
          if(index + 1 < partitionSizes[partition])
          {
            ++index;
          }
          else
          {
            index = index_type{};
            ++partition;
            while(!partitionSizes[partition]) ++partition;
          }
                                                  
          --i;
        }
        
        return SharingPolicy::make(*((list.begin() + partition)->begin() + index));
      }

      template<class PartitionSizes, size_type... Inds>
      constexpr static StorageType make_array(std::integer_sequence<size_type, Inds...>, std::initializer_list<std::initializer_list<T>> list, const PartitionSizes& partitionSizes)
      {
        if(list.size() != container_type::num_partitions())
          throw std::logic_error("Overall initializer list of wrong size");

        size_type total{};
        for(auto l : list) total += l.size();
        
        if(total != container_type::num_elements())
          throw std::logic_error("Inconsistent number of elements supplied by initializer lists");

        if constexpr(sizeof...(Inds) > 0)
        {
          return StorageType{ make_element(Inds, list, partitionSizes)... };
        }
        else
        {
          return StorageType{};
        }
      }

      /*
      template<size_type N> constexpr static index_type get_separator_element(const size_type i, const std::array<index_type, N>& sizeArray)
      {
        index_type sep{};
        for(size_type j{}; j<=i; ++j)
          sep += sizeArray[j];

        return sep;
        }*/

      constexpr static auto make_size_array(std::initializer_list<std::initializer_list<T>> list)
      {
        constexpr auto N{container_type::num_partitions()};
        return utilities::to_array<index_type, N>(list, [](const auto& l){ return l.size(); });
      }


      template<std::size_t N>
      constexpr static auto make_separator_array(std::array<index_type, N> sizeArray)
      {
        if(!sizeArray.empty())
        {
          for(auto iter{sizeArray.begin()+1}; iter!=sizeArray.end(); ++iter)
          {
            *iter += * (iter - 1);
          }
        }

        return sizeArray;
      }

      /*
      template<size_type... Inds>
      constexpr static std::array<index_type, sizeof...(Inds)>
      make_separator_array(std::integer_sequence<size_type, Inds...>, std::initializer_list<std::initializer_list<T>> list)
      {        
        return { get_separator_element(Inds, make_size_array(list))... };
        }*/


      constexpr contiguous_storage_base(std::true_type, std::initializer_list<std::initializer_list<T>> list)
        : m_Storage{make_array(std::make_index_sequence<container_type::num_elements()>{}, list, make_size_array(list))}
      , m_Partitions{make_separator_array(make_size_array(list))}
      {
      }
      
      contiguous_storage_base(std::false_type, std::initializer_list<std::initializer_list<T>> list)
      {
        for(auto iter = list.begin(); iter != list.end(); ++iter)
        { 
          add_slot();
          const auto dist{std::distance(list.begin(), iter)};
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
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

      template<class... Args>
      auto insert(const index_type index, Args&&... args)
      {
        auto iter = m_Storage.end();
        if(index == m_Partitions.size() - 1)
        {
          m_Storage.push_back(SharingPolicy::make(std::forward<Args>(args)...));
          iter = --m_Storage.end();
        }
        else
        {
          iter = m_Storage.insert(m_Storage.begin() + m_Partitions[index], SharingPolicy::make(std::forward<Args>(args)...));
        }

        for(size_type i{index}; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }

        return iter;
      }

      partition_iterator insert_directly_to_partition(const index_type index, const index_type pos, const held_type& toAdd)
      {
        if constexpr(throw_on_range_error) check_range(index, pos);

        auto iter = m_Storage.begin();
        const index_type offset{(index > index_type{}) ? m_Partitions[index - 1] + pos : pos};

        iter = m_Storage.insert(iter + offset, toAdd);

        for(auto i = index; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }

        return partition_iterator(iter, index);
      }

      template<class PartitionIterator, class Iterator>
      constexpr PartitionIterator get_begin_iterator(const index_type i, Iterator iter) const noexcept
      {
        const index_type index(i >= m_Partitions.size() ? npos : i);
        const auto offset = (i > 0 && i < m_Partitions.size()) ? m_Partitions[i-1] : index_type{};
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
      constexpr static bool throw_on_range_error{true};
      template<class S> using underlying_storage_type = std::vector<S, std::allocator<S>>; 
    };

    template<class T, class SharingPolicy=data_sharing::independent<T>, class Traits=contiguous_storage_traits<T, SharingPolicy>>
    class contiguous_storage : public contiguous_storage_base<T, SharingPolicy, Traits>
    {
    private:
      using base_t = contiguous_storage_base<T, SharingPolicy, Traits>;
    public:
      using contiguous_storage_base<T, SharingPolicy, Traits>::contiguous_storage_base;

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
      using base_t::erase_from_partition_if;
    };

    template<class T, std::size_t Npartitions, std::size_t Nelements, class IndexType> struct static_contiguous_storage_traits
    {
      constexpr static bool throw_on_range_error{true};
      template<class S> using underlying_storage_type = typename impl::static_contiguous_data<Npartitions,Nelements,std::make_unsigned_t<IndexType>>::template data<S>; 
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
    

    template
    <
      class Storage1, class Storage2
    >
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
}
