#pragma once

#include "StaticData.hpp"
#include "Utilities.hpp"
#include "Iterator.hpp"

#include <map>
#include <memory>
#include <limits>
#include <string>

#include "SharingPolicies.hpp"

namespace sequoia
{
  namespace data_structures
  {
    template<class T> struct const_reference
    {
      using type      = const T;
      using reference = const T&;
      using pointer   = const T*;
    };

    template <class T> struct mutable_reference
    {
      using type      = T;
      using reference = T&;
      using pointer   = T*;
    };

    template<template<class...> class C, class SharingPolicy> struct storage_type_generator
    {
      using held_type = typename SharingPolicy::handle_type;
      using container_type = C<held_type>;
      using helper = storage_helper<container_type>;
      using storage_type = typename helper::storage_type;
      using auxiliary_storage_type = typename helper::auxiliary_storage_type;
      using static_type = typename std::is_base_of<static_data_base, C<held_type>>::type;
    };
    
    template<template<class...> class C, class SharingPolicy, template<class> class ReferencePolicy, bool Reversed>
    struct partition_iterator_helper
    {
      using iterator = typename storage_type_generator<C, SharingPolicy>::storage_type::iterator;
    };

    template<template<class...> class C, class SharingPolicy>
    struct partition_iterator_helper<C, SharingPolicy, mutable_reference, true>
    {
      using iterator = typename storage_type_generator<C, SharingPolicy>::storage_type::reverse_iterator;
    };
    
    template<template<class...> class C, class SharingPolicy>
    struct partition_iterator_helper<C, SharingPolicy, const_reference, false>
    {
      using iterator = typename storage_type_generator<C, SharingPolicy>::storage_type::const_iterator;
    };

    template<template<class...> class C, class SharingPolicy>
    struct partition_iterator_helper<C, SharingPolicy, const_reference, true>
    {
      using iterator = typename storage_type_generator<C, SharingPolicy>::storage_type::const_reverse_iterator;
    };

    namespace partition_impl
    {
      template<bool Reversed, class IndexType>
      class partition_index_policy
      {
      public:
        using index_type = IndexType;

        constexpr static index_type npos{std::numeric_limits<index_type>::max()};
        
        constexpr static bool reversed() { return Reversed; }

        friend constexpr bool operator==(const partition_index_policy& lhs, const partition_index_policy& rhs)
        {
          return lhs.m_Partition == rhs.m_Partition;
        }

        friend constexpr bool operator!=(const partition_index_policy& lhs, const partition_index_policy& rhs)
        {
          return !(lhs == rhs);
        }

        constexpr IndexType partition_index() const noexcept { return m_Partition; }
      protected:        
        constexpr partition_index_policy(const index_type n) : m_Partition{n} {}        
        ~partition_index_policy() = default;
        
        constexpr partition_index_policy(const partition_index_policy&)                = default;
        constexpr partition_index_policy(partition_index_policy&&) noexcept            = default;
        constexpr partition_index_policy& operator=(const partition_index_policy&)     = default;
        constexpr partition_index_policy& operator=(partition_index_policy&&) noexcept = default;
      private:
        IndexType m_Partition;
      };

      template<class SharingPolicy, template<class> class ReferencePolicy>
      struct dereference_policy : public SharingPolicy
      {
        using elementary_type = typename SharingPolicy::elementary_type;
        using reference       = typename ReferencePolicy<elementary_type>::reference;
        using pointer         = typename ReferencePolicy<elementary_type>::pointer;
      };
    }

    //===================================A Custom Iterator===================================//

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using partition_iterator = utilities::iterator<typename partition_iterator_helper<C, SharingPolicy, mutable_reference, false>::iterator, partition_impl::dereference_policy<SharingPolicy, mutable_reference>, partition_impl::partition_index_policy<false, IndexType>>;

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using const_partition_iterator = utilities::iterator<typename partition_iterator_helper<C, SharingPolicy, const_reference, false>::iterator, partition_impl::dereference_policy<SharingPolicy, const_reference>, partition_impl::partition_index_policy<false, IndexType>>;

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using reverse_partition_iterator = utilities::iterator<typename partition_iterator_helper<C, SharingPolicy, mutable_reference, true>::iterator, partition_impl::dereference_policy<SharingPolicy, mutable_reference>, partition_impl::partition_index_policy<true, IndexType>>;

    template<template<class...> class C, class SharingPolicy, class IndexType>
    using const_reverse_partition_iterator = utilities::iterator<typename partition_iterator_helper<C, SharingPolicy, const_reference, true>::iterator, partition_impl::dereference_policy<SharingPolicy, const_reference>, partition_impl::partition_index_policy<true, IndexType>>;

    //============================= Helper classes for copy constructor ===================================//

    template<class T> class data_duplicator
    {
    };
    
    template<class T> class data_duplicator<data_sharing::independent<T>>
    {
    public:
      constexpr T duplicate(const T& in) const { return T{in}; }
    private:
    };

    template<class T> class data_duplicator<data_sharing::shared<T>>
    {
    public:
      using handle_type = typename data_sharing::shared<T>::handle_type;

      handle_type duplicate(const handle_type in)
      {
        handle_type ptr{};
        auto found = m_ProcessedPointers.find(in);
        if(found == m_ProcessedPointers.end())
        {
          ptr = data_sharing::shared<T>::make(*in);
          m_ProcessedPointers.insert(make_pair(in, ptr));
        }
        else
        {
          ptr = found->second;
        }
        return ptr;
      }
    private:
      std::map<handle_type, handle_type> m_ProcessedPointers;
    };
    
    //===================================Storage using buckets===================================//
    
    template<class T, class SharingPolicy=data_sharing::independent<T>, bool ThrowOnRangeError=true, template<class...> class Storage=std::vector>
    class bucketed_storage
    {
    public:
      using value_type = T;
      using sharing_policy_type = SharingPolicy;
      using partition_iterator = partition_iterator<Storage, SharingPolicy, std::size_t>;
      using const_partition_iterator = const_partition_iterator<Storage, SharingPolicy, std::size_t>;
      using reverse_partition_iterator = reverse_partition_iterator<Storage, SharingPolicy, std::size_t>;
      using const_reverse_partition_iterator = const_reverse_partition_iterator<Storage, SharingPolicy, std::size_t>;

      constexpr static bool throw_on_range_error() { return ThrowOnRangeError; }
      
      bucketed_storage() {}

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
        data_duplicator<SharingPolicy> duplicator;
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

      std::size_t size() const
      {
        std::size_t size{};
        for(const auto& bucket : m_Buckets)
        {
          size += bucket.size();
        }

        return size;
      }

      void add_slot()
      {
        m_Buckets.push_back(std::vector<held_type>());
      }

      std::size_t insert_slot(const std::size_t pos)
      {
        std::size_t partition{pos};
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

      std::size_t delete_slot(const std::size_t n)
      {
        std::size_t deleted{};
        if(n < m_Buckets.size())
        {
          deleted = m_Buckets[n].size();
          m_Buckets.erase(m_Buckets.begin() + n);
        }

        return deleted;
      }

      std::size_t num_partitions() const noexcept { return m_Buckets.size(); }

      void reserve(const std::size_t partition, const std::size_t size)
      {
        if constexpr(ThrowOnRangeError) check_range(partition);

        m_Buckets[partition].reserve(size);
      }

      std::size_t capacity(const std::size_t partition) const
      {
        if constexpr(ThrowOnRangeError) check_range(partition);

        return m_Buckets[partition].capacity();
      }

      void reserve_partitions(const std::size_t numPartitions)
      {
        m_Buckets.reserve(numPartitions);
      }

      std::size_t num_partitions_capacity() const noexcept
      {

        return m_Buckets.capacity();
      }

      void shrink_num_partitions_to_fit()
      {
        m_Buckets.shrink_to_fit();
      }

      void shrink_to_fit(const std::size_t partition)
      {
        if constexpr(ThrowOnRangeError) check_range(partition);

        m_Buckets[partition].shrink_to_fit;
      }
      
      void shrink_to_fit()
      {
        m_Buckets.shrink_to_fit();
        for(auto& b : m_Buckets)
        {
          b.shrink_to_fit();
        }
      }

      void clear() noexcept
      {
        for(auto& bucket : m_Buckets) bucket.clear();
      }
      
      template<class... Args>
      void push_back_to_partition(const std::size_t index, Args&&... args)
      {
        if constexpr(ThrowOnRangeError) check_range(index);

        m_Buckets[index].push_back(SharingPolicy::make(std::forward<Args>(args)...));
      }

      void push_back_to_partition(const std::size_t index, const_partition_iterator iter)
      {
        if constexpr(ThrowOnRangeError) check_range(index);

        m_Buckets[index].push_back(*(iter.base_iterator()));
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        if constexpr(ThrowOnRangeError)
        {
          if(!m_Buckets.size()) throw std::runtime_error("bucketed_storage: no partitions into which to insert");
        }
        
        const auto host{pos.partition_index()};
        const auto index{static_cast<std::size_t>(distance(cbegin_partition(host), pos))};        
        auto iter{m_Buckets[host].insert(pos.base_iterator(), SharingPolicy::make(std::forward<Args>(args)...))};

        return partition_iterator{iter, index};
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, const_partition_iterator setFromIter)
      {
        if constexpr(ThrowOnRangeError)
        {
          if(!m_Buckets.size()) throw std::runtime_error("bucketed_storage: no partitions into which to insert");
        }
        
        const auto host{pos.partition_index()};
        const auto index{static_cast<std::size_t>(distance(cbegin_partition(host), pos))};        
        auto iter{m_Buckets[host].insert(pos.base_iterator(), *(setFromIter.base_iterator()))};

        return partition_iterator{iter, index};
      }

      partition_iterator delete_from_partition(const std::size_t index, const std::size_t pos)
      {
        auto iter = begin_partition(index);
        auto next = end_partition(index).base_iterator();
        if(iter != end_partition(index) && pos < static_cast<std::size_t>(distance(iter, end_partition(index))))
        {
          next = m_Buckets[index].erase((iter + pos).base_iterator());
        }

        return partition_iterator{next, index};
      }

      partition_iterator delete_from_partition(const_partition_iterator iter)
      {
        const auto partition = iter.partition_index();
        const auto next = m_Buckets[partition].erase(iter.base_iterator());
        return partition_iterator{next, partition};
      }

      template<class UnaryPred>
      std::size_t delete_from_partition_if(const std::size_t partitionIndex, UnaryPred pred)
      {
        std::size_t deleted{};
        auto iter = begin_partition(partitionIndex);
        while(iter != end_partition(partitionIndex))
        {
          if(pred(*iter))
          {
            iter = partition_iterator(m_Buckets[partitionIndex].erase(iter.base_iterator()), partitionIndex);
            ++deleted;
          }
          else
          {
            ++iter;
          }
        }

        return deleted;
      }

      template<class UnaryPred>
      partition_iterator find_in_partition_if(const std::size_t partitionIndex, UnaryPred pred)
      {
        auto found = end_partition(partitionIndex);
        for(auto iter = begin_partition(partitionIndex); iter != end_partition(partitionIndex); ++iter)
        {
          if(pred(*iter)) return(iter);
        }

        return found;
      }

      template<class UnaryPred>
      const_partition_iterator find_in_partition_if(const std::size_t partitionIndex, UnaryPred pred) const
      {
          auto found = cend_partition(partitionIndex);
          for(auto iter = cbegin_partition(partitionIndex); iter != cend_partition(partitionIndex); ++iter)
          {
              if(pred(*iter)) return(iter);
          }

          return found;
      }

      partition_iterator begin_partition(const std::size_t i)
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? partition_iterator(m_Buckets[i].begin(), i) : partition_iterator(m_Buckets.back().end(), npos);
      }

      partition_iterator end_partition(const std::size_t i)
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? partition_iterator(m_Buckets[i].end(), i) : partition_iterator(m_Buckets.back().end(), npos);
      }

      const_partition_iterator begin_partition(const std::size_t i) const
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_partition_iterator(m_Buckets[i].cbegin(), i) : const_partition_iterator(m_Buckets.back().cend(), npos);
      }

      const_partition_iterator end_partition(const std::size_t i) const
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_partition_iterator(m_Buckets[i].cend(), i) : const_partition_iterator(m_Buckets.back().cend(), npos);
      }

      reverse_partition_iterator rbegin_partition(const std::size_t i)
      {
        if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? reverse_partition_iterator(m_Buckets[i].rbegin(), i) : reverse_partition_iterator(m_Buckets.front().rend(), npos);
      }

      reverse_partition_iterator rend_partition(const std::size_t i)
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? reverse_partition_iterator(m_Buckets[i].rend(), i) : reverse_partition_iterator(m_Buckets.front().rend(), npos);
      }

      const_reverse_partition_iterator rbegin_partition(const std::size_t i) const
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator(m_Buckets[i].crbegin(), i) : const_reverse_partition_iterator(m_Buckets.front().crend(), npos);

      }

      const_reverse_partition_iterator rend_partition(const std::size_t i) const
      {
        if constexpr(ThrowOnRangeError) if(m_Buckets.empty()) throw std::out_of_range("bucketed_storage::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator(m_Buckets[i].crend(), i) : const_reverse_partition_iterator(m_Buckets.front().crend(), npos);
      }

      const_partition_iterator cbegin_partition(const std::size_t i) const
      {
        return begin_partition(i);
      }

      const_partition_iterator cend_partition(const std::size_t i) const
      {
        return end_partition(i);
      }

      const_reverse_partition_iterator crbegin_partition(const std::size_t i) const
      {
        return rbegin_partition(i);
      }

      const_reverse_partition_iterator crend_partition(const std::size_t i) const
      {
        return rend_partition(i);
      }
      
      const_partition_iterator operator[](const std::size_t i) const { return cbegin_partition(i); }
      partition_iterator operator[](const std::size_t i) { return begin_partition(i); }
    private:
      using held_type = typename SharingPolicy::handle_type;
      constexpr static std::size_t npos{partition_iterator::npos};

      Storage<Storage<held_type>> m_Buckets;

      void check_range(const std::size_t index) const
      {
        if(index >= m_Buckets.size())
        {
          throw std::out_of_range("bucketed_storage::index " + std::to_string(index) + " out of range!");
        }
      }

      void check_range(const std::size_t index, const std::size_t pos) const
      {
        check_range(index);
        const std::size_t bucketSize{m_Buckets[index].size()};
        if(pos > bucketSize)
        {
          throw std::out_of_range("bucketed_storage::partition " + std::to_string(index) + " pos " + std::to_string(pos) + " out of range");
        }
      }

      partition_iterator insert_directly_to_partition(const std::size_t index, const std::size_t pos, const held_type& toAdd)
      {
        if constexpr(ThrowOnRangeError)
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
    
    template<class T, class SharingPolicy=data_sharing::independent<T>, bool ThrowOnRangeError=true, template<class...> class Storage=std::vector>
    class contiguous_storage_base
    {
    private:
      using AuxiliaryType  = typename storage_type_generator<Storage, SharingPolicy>::auxiliary_storage_type;
    public:
      using value_type          = T;
      using sharing_policy_type = SharingPolicy;
      using index_type          = typename AuxiliaryType::value_type;

      using partition_iterator               = partition_iterator<Storage, SharingPolicy, index_type>;
      using const_partition_iterator         = const_partition_iterator<Storage, SharingPolicy, index_type>;
      using reverse_partition_iterator       = reverse_partition_iterator<Storage, SharingPolicy, index_type>;
      using const_reverse_partition_iterator = const_reverse_partition_iterator<Storage, SharingPolicy, index_type>;

      constexpr static bool throw_on_range_error() { return ThrowOnRangeError; }
      
      contiguous_storage_base() {}

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

      constexpr std::size_t size() const noexcept { return m_Storage.size(); }

      constexpr std::size_t num_partitions() const noexcept { return m_Partitions.size(); }

      template<class UnaryPred>
      partition_iterator find_in_partition_if(const index_type partitionIndex, UnaryPred pred)
      {
        auto found = end_partition(partitionIndex);
        for(auto iter = begin_partition(partitionIndex); iter != end_partition(partitionIndex); ++iter)
        {
          if(pred(*iter)) return(iter);
        }

        return found;
      }

      template<class UnaryPred>
      const_partition_iterator find_in_partition_if(const index_type partitionIndex, UnaryPred pred) const
      {
          auto found = cend_partition(partitionIndex);
          for(auto iter = cbegin_partition(partitionIndex); iter != cend_partition(partitionIndex); ++iter)
          {
              if(pred(*iter)) return(iter);
          }

          return found;
      }
      
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

      constexpr const_partition_iterator operator[](const index_type i) const { return cbegin_partition(i); }
      partition_iterator operator[](const index_type i) { return begin_partition(i); }
    protected:
      void add_slot()
      {
        m_Partitions.push_back(m_Storage.size());
      }

      std::size_t insert_slot(const std::size_t pos)
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

      index_type delete_slot(const index_type n)
      {
        index_type deleted{};
        if(n < m_Partitions.size())
        {
          const index_type offset{(n > 0) ? m_Partitions[n - 1] : index_type{}};
          if(m_Partitions[n] > 0)
          {
            deleted = m_Partitions[n] - offset;
            m_Storage.erase(m_Storage.begin() + offset, m_Storage.begin() + m_Partitions[n]);
          }
          m_Partitions.erase(m_Partitions.begin() + n);
          for(std::size_t i=n; i < m_Partitions.size(); ++i)
          {
            m_Partitions[i]-=deleted;
          }
        }

        return deleted;
      }

      void reserve(const index_type size)
      {
        m_Storage.reserve(size);
      }

      index_type capacity() const noexcept
      {
        return m_Storage.capacity();
      }

      void reserve_partitions(const index_type numPartitions)
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
        if constexpr(ThrowOnRangeError) check_range(index);
        insert(index, std::forward<Args>(args)...);
      }

      // Unify this with insert method
      void push_back_to_partition(const index_type index, const_partition_iterator iter)
      {
        if constexpr(ThrowOnRangeError) check_range(index);
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

        for(std::size_t i=index; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        if constexpr(ThrowOnRangeError)
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
        if constexpr(ThrowOnRangeError)
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

      partition_iterator delete_from_partition(const index_type index, const std::size_t pos)
      {
        auto next = end_partition(index).base_iterator();
        auto iter = begin_partition(index);
        if(iter != end_partition(num_partitions() - 1) && pos < static_cast<std::size_t>(distance(iter, end_partition(index))))
        {
          next = m_Storage.erase((iter + pos).base_iterator());
          for(std::size_t i=index; i < m_Partitions.size(); ++i)
          {
            m_Partitions[i]-=1;
          }
        }

        return partition_iterator(next, index);
      }

      partition_iterator delete_from_partition(const_partition_iterator iter)
      {
        const auto next = m_Storage.erase(iter.base_iterator());
        const auto index = iter.partition_index();
        for(std::size_t i=index; i < m_Partitions.size(); ++i)
        {
          m_Partitions[i]-=1;
        }
        return partition_iterator{next, iter.partition_index()};
      }

      template<class UnaryPred>
      index_type delete_from_partition_if(const index_type partitionIndex, UnaryPred pred)
      {
        index_type deleted{};
        auto iter = begin_partition(partitionIndex);
        while(iter != end_partition(partitionIndex))
        {
          if(pred(*iter))
          {
            iter = partition_iterator(m_Storage.erase(iter.base_iterator()), partitionIndex);
            ++deleted;

            for(std::size_t i=partitionIndex; i < m_Partitions.size(); ++i)
            {
              --m_Partitions[i];
            }
          }
          else
          {
            ++iter;
          }
        }

        return deleted;
      }

    private:
      using held_type      = typename storage_type_generator<Storage, SharingPolicy>::held_type;
      using container_type = typename storage_type_generator<Storage, SharingPolicy>::container_type;
      using StorageType    = typename storage_type_generator<Storage, SharingPolicy>::storage_type;      
      using StaticType     = typename storage_type_generator<Storage, SharingPolicy>::static_type;
      constexpr static index_type npos{partition_iterator::npos};
      
      StorageType m_Storage;
      AuxiliaryType m_Partitions;

      constexpr contiguous_storage_base(std::false_type, const contiguous_storage_base& in) : m_Partitions{in.m_Partitions}
      {
        data_duplicator<SharingPolicy> duplicator;
        for(const auto& elt : in.m_Storage)
        {
          m_Storage.push_back(duplicator.duplicate(elt));
        }
      }

      constexpr contiguous_storage_base(std::true_type, const contiguous_storage_base& in) : m_Storage{in.m_Storage}, m_Partitions{in.m_Partitions}
      {
      }

      template<class PartitionSizes>
      constexpr static held_type make_element(std::size_t i, std::initializer_list<std::initializer_list<T>> list, const PartitionSizes& partitionSizes)
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

      template<class PartitionSizes, std::size_t... Inds>
      constexpr static StorageType make_array(std::integer_sequence<std::size_t, Inds...>, std::initializer_list<std::initializer_list<T>> list, const PartitionSizes& partitionSizes)
      {
        if(list.size() != container_type::num_partitions())
          throw std::logic_error("Overall initializer list of wrong size");

        std::size_t total{};
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

      constexpr static index_type get_size(const std::size_t i, std::initializer_list<std::initializer_list<T>> list)
      {
        return index_type((list.begin() + i)->size());
      }

      template<std::size_t... Inds>
      constexpr static std::array<index_type, sizeof...(Inds)>
      make_size_array(std::integer_sequence<std::size_t, Inds...>, std::initializer_list<std::initializer_list<T>> list)
      {
        return { get_size(Inds, list)... };
      }

      template<std::size_t... Inds>
      constexpr static std::array<index_type, sizeof...(Inds)>
      make_separator_array(std::integer_sequence<std::size_t, Inds...>, std::initializer_list<std::initializer_list<T>> list)
      {
        const auto sizeArray = make_size_array(std::make_index_sequence<container_type::num_partitions()>{}, list);
        return { get_separator_element(Inds, sizeArray)... };
      }

      template<std::size_t N> constexpr static index_type get_separator_element(const std::size_t i, const std::array<index_type, N>& sizeArray)
      {
        index_type sep{};
        for(std::size_t j{}; j<=i; ++j)
          sep += sizeArray[j];

        return sep;
      }

      constexpr contiguous_storage_base(std::true_type, std::initializer_list<std::initializer_list<T>> list)
        : m_Storage{make_array(std::make_index_sequence<container_type::num_elements()>{}, list, make_size_array(std::make_index_sequence<container_type::num_partitions()>{}, list))}
        , m_Partitions{make_separator_array(std::make_index_sequence<container_type::num_partitions()>{}, list)}
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

      void check_range(const std::size_t index) const
      {
        if(index >= m_Partitions.size())
        {
          throw std::out_of_range("contiguous_storage::index" + std::to_string(index) +  "out of range");
        }
      }

      void check_range(const std::size_t index, const index_type pos) const
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

        for(std::size_t i=index; i < m_Partitions.size(); ++i)
        {
          ++m_Partitions[i];
        }

        return iter;
      }

      partition_iterator insert_directly_to_partition(const index_type index, const index_type pos, const held_type& toAdd)
      {
        if constexpr(ThrowOnRangeError) check_range(index, pos);

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

    template<class T, class SharingPolicy=data_sharing::independent<T>, bool ThrowOnRangeError=true, template<class...> class Storage=std::vector>
    class contiguous_storage : public contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>
    {
    public:
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::contiguous_storage_base;

      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::add_slot;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::insert_slot;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::delete_slot;

      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::reserve;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::capacity;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::reserve_partitions;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::num_partitions_capacity;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::shrink_to_fit;
      
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::clear;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::push_back_to_partition;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::insert_to_partition;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::delete_from_partition;
      using contiguous_storage_base<T, SharingPolicy, ThrowOnRangeError, Storage>::delete_from_partition_if;
    };

    template<class T, std::size_t Npartitions, std::size_t Nelements, bool ThrowOnRangeError=true, class IndexType=std::size_t>
    class static_contiguous_storage
      : public contiguous_storage_base<T, data_sharing::independent<T>, ThrowOnRangeError, static_contiguous_data<Npartitions,Nelements,IndexType>::template data>
    {
    public:
      using contiguous_storage_base<
        T,
        data_sharing::independent<T>,
        ThrowOnRangeError,
        static_contiguous_data<Npartitions,Nelements,IndexType>::template data
      >::contiguous_storage_base;
    };
    

    template
    <
      class T,
      class FirstSharingPolicy, class SecondSharingPolicy,
      template<class...> class FirstStoragePolicy, template<class...> class SecondStoragePolicy,
      bool FirstThrowPolicy, bool SecondThrowPolicy,
      template<class, class, bool, template<class...> class> class FirstStorageType,
      template<class, class, bool, template<class...> class> class SecondStorageType
    >
    bool operator==(const FirstStorageType<T, FirstSharingPolicy, FirstThrowPolicy, FirstStoragePolicy>& first,
                    const SecondStorageType<T, SecondSharingPolicy, SecondThrowPolicy, SecondStoragePolicy>& second) noexcept
    {
      if(first.num_partitions() != second.num_partitions()) return false;

      for(std::size_t i{}; i < first.num_partitions(); ++i)
      {
        if(distance(first.cbegin_partition(i), first.cend_partition(i))
          != distance(second.cbegin_partition(i), second.cend_partition(i)))
        {
          return false;
        }

        auto iter1 = first.cbegin_partition(i);
        auto iter2 = second.cbegin_partition(i);
        for(; iter1 != first.cend_partition(i); ++iter1, ++iter2)
        {
          if(*iter1 != *iter2) return false;
        }
      }

      return true;
    }

    template
    <
      class T,
      class FirstSharingPolicy, class SecondSharingPolicy,
      template<class...> class FirstStoragePolicy, template<class...> class SecondStoragePolicy,
      bool FirstThrowPolicy, bool SecondThrowPolicy,
      template<class, class, bool, template<class...> class> class FirstStorageType,
      template<class, class, bool, template<class...> class> class SecondStorageType
    >
    bool operator!=(FirstStorageType<T, FirstSharingPolicy, FirstThrowPolicy, FirstStoragePolicy>& first,
                    SecondStorageType<T, SecondSharingPolicy, SecondThrowPolicy, SecondStoragePolicy>& second) noexcept
   {
     return !(first == second);
   }
  }
}
