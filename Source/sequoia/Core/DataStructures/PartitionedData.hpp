////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Classes implementing the concept of a sequence of data which is divided into partitions.
 */

#include "sequoia/Core/DataStructures/PartitionedDataDetails.hpp"
#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Core/ContainerUtilities/Iterator.hpp"
#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Maths/Sequences/MonotonicSequence.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <string>
#include <numeric>
#include <stdexcept>

namespace sequoia
{
  namespace data_structures
  {
    //===================================A Custom Iterator===================================//

    template<class Container, std::integral IndexType>
    using partition_iterator
      = utilities::iterator<
          typename Container::iterator,
          utilities::identity_dereference_policy<typename Container::iterator, partition_impl::partition_index_policy<false, IndexType>>>;

    template<class Container, std::integral IndexType>
    using const_partition_iterator
      = utilities::iterator<
          typename Container::const_iterator,
          utilities::identity_dereference_policy<typename Container::const_iterator, partition_impl::partition_index_policy<false, IndexType>>>;

    template<class Container, std::integral IndexType>
    using reverse_partition_iterator
      = utilities::iterator<
          typename Container::reverse_iterator,
          utilities::identity_dereference_policy<typename Container::reverse_iterator, partition_impl::partition_index_policy<true, IndexType>>>;

    template<class Container, std::integral IndexType>
    using const_reverse_partition_iterator
      = utilities::iterator<
          typename Container::const_reverse_iterator,
          utilities::identity_dereference_policy<typename Container::const_reverse_iterator, partition_impl::partition_index_policy<true, IndexType>>>;


    //===================================Storage using buckets===================================//

    /*! \brief Storage for partitioned data such that data within each partition is contiguous.
     */

    template<class T, class Container=std::vector<std::vector<T>>>
    class bucketed_sequence
    {
    public:
      using value_type     = T;
      using container_type = Container;
      using bucket_type    = typename container_type::value_type;
      using size_type      = typename container_type::size_type;
      using index_type     = size_type;
      using allocator_type = typename container_type::allocator_type;
      //using partitions_allocator_type = typename bucket_type::allocator_type;

      using partition_iterator               = data_structures::partition_iterator<bucket_type, size_type>;
      using const_partition_iterator         = data_structures::const_partition_iterator<bucket_type, size_type>;
      using reverse_partition_iterator       = data_structures::reverse_partition_iterator<bucket_type, size_type>;
      using const_reverse_partition_iterator = data_structures::const_reverse_partition_iterator<bucket_type, size_type>;
      using partition_range                  = std::ranges::subrange<partition_iterator>;
      using const_partition_range            = std::ranges::subrange<const_partition_iterator>;

      constexpr static auto npos{partition_iterator::npos};

      bucketed_sequence() noexcept(noexcept(allocator_type{})) = default;

      explicit bucketed_sequence(const allocator_type& allocator) noexcept
        : m_Buckets(allocator)
      {}

      bucketed_sequence(std::initializer_list<std::initializer_list<T>> list,
                       const allocator_type& allocator = allocator_type{})
        : m_Buckets(allocator)
      {
        m_Buckets.reserve(list.size());
        for(auto iter{list.begin()}; iter != list.end(); ++iter)
        {
          add_slot();
          const auto dist{std::ranges::distance(list.begin(), iter)};
          m_Buckets[dist].reserve(iter->size());
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
      }

      bucketed_sequence(const bucketed_sequence&) = default;

      bucketed_sequence(const bucketed_sequence& other, const allocator_type& allocator)
       : m_Buckets(std::allocator_traits<allocator_type>::select_on_container_copy_construction(allocator))
      {
        m_Buckets.reserve(other.m_Buckets.size());

        for(auto i{other.m_Buckets.cbegin()}; i != other.m_Buckets.cend(); ++i)
        {
          const auto& bucket{*i};
          const auto dist{std::ranges::distance(other.m_Buckets.cbegin(), i)};
          add_slot();
          m_Buckets.back().reserve(other.m_Buckets[dist].size());
          for(const auto& elt : bucket)
          {
            m_Buckets.back().push_back(elt);
          }
        }
      }

      bucketed_sequence(bucketed_sequence&&) noexcept = default;

      bucketed_sequence(bucketed_sequence&& other, const allocator_type& allocator)
        : m_Buckets(std::move(other).m_Buckets, allocator)
      {}

      bucketed_sequence& operator=(bucketed_sequence&&) noexcept = default;

      bucketed_sequence& operator=(const bucketed_sequence&) = default;

      void swap(bucketed_sequence& other)
        noexcept(noexcept(std::ranges::swap(this->m_Buckets, other.m_Buckets)))
      {
        std::ranges::swap(m_Buckets, other.m_Buckets);
      }

      friend void swap(bucketed_sequence& lhs, bucketed_sequence& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
      {
        lhs.swap(rhs);
      }

      [[nodiscard]]
      bool empty() const noexcept
      {
        return m_Buckets.empty();
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
      size_type size_of_partition(size_type i) const
      {
        return static_cast<size_type>(std::ranges::distance(partition(i)));
      }

      [[nodiscard]]
      size_type num_partitions() const noexcept { return m_Buckets.size(); }

      void swap_partitions(const size_type i, const size_type j)
      {
        if((i < num_partitions()) && (j < num_partitions()))
        {
          if(i != j) std::ranges::swap(m_Buckets[i], m_Buckets[j]);
        }
      }

      [[nodiscard]]
      allocator_type get_allocator() const
      {
        return m_Buckets.get_allocator();
      }

      void add_slot()
      {
        m_Buckets.emplace_back();
      }

      void insert_slot(const size_type pos)
      {
        if(pos < num_partitions())
        {
          auto iter{m_Buckets.begin() + pos};
          m_Buckets.emplace(iter);
        }
        else
        {
          add_slot();
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
        if(partition < num_partitions())
          m_Buckets[partition].reserve(size);
      }

      [[nodiscard]]
      size_type partition_capacity(const size_type partition) const
      {
        return partition < num_partitions() ? m_Buckets[partition].capacity() : 0;
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
        if(partition < num_partitions()) m_Buckets[partition].shrink_to_fit();
      }

      void shrink_to_fit()
      {
        shrink_num_partitions_to_fit();
        for(auto& b : m_Buckets) b.shrink_to_fit();
      }

      void clear() noexcept
      {
        m_Buckets.clear();
      }

      template<class... Args>
      void push_back_to_partition(const size_type index, Args&&... args)
      {
        check_range("push_back_to_partition", index);

        m_Buckets[index].emplace_back(std::forward<Args>(args)...);
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        const auto source{pos.partition_index()};
        check_range("insert_to_partition", source);

        auto iter{m_Buckets[source].emplace(pos.base_iterator(), std::forward<Args>(args)...)};

        return partition_iterator{iter, source};
      }

      template<class... Args>
      partition_iterator insert_to_partition(const size_type index, const size_type pos, Args&&... args)
      {
        check_range("insert_to_partition", index, pos);
        return insert_to_partition(std::ranges::next(begin_partition_raw(index), pos, end_partition_raw(index)), std::forward<Args>(args)...);
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto partition{iter.partition_index()};
        if(const auto n{num_partitions()}; partition >= n)
        {
          return end_partition(n);
        }
        else if(iter == cend_partition(partition))
        {
          return end_partition(partition);
        }

        const auto next{m_Buckets[partition].erase(iter.base_iterator())};
        return {next, partition};
      }

      partition_iterator erase_from_partition(const_partition_iterator first, const_partition_iterator last)
      {
        const auto firstPartition{first.partition_index()}, lastPartition{last.partition_index()};
        if(const auto n{num_partitions()}; (firstPartition >= n) && (lastPartition >= n))
        {
          return end_partition(n);
        }
        else if(lastPartition == firstPartition)
        {
          if(std::ranges::distance(first, last) > 0)
          {
            const auto next{m_Buckets[firstPartition].erase(first.base_iterator(), last.base_iterator())};
            return {next, firstPartition};
          }

          return std::ranges::next(begin_partition(firstPartition), std::ranges::distance(cbegin_partition(firstPartition), first));
        }

        throw std::domain_error{std::string{"bucketed_sequence::erase - Invalid range specified by iterators belonging to partitions ["}
        .append(std::to_string(firstPartition)).append(", ").append(std::to_string(lastPartition)).append("]")};
      }

      partition_iterator erase_from_partition(const size_type index, const size_type pos)
      {
        check_for_empty("erase_from_partition");
        return erase_from_partition(std::ranges::next(begin_partition_raw(index), pos, end_partition_raw(index)));
      }

      [[nodiscard]]
      partition_iterator begin_partition(const size_type i)
      {
        check_for_empty("begin_partition");
        return (i < m_Buckets.size()) ? partition_iterator{m_Buckets[i].begin(), i} : partition_iterator{m_Buckets.back().end(), npos};
      }

      [[nodiscard]]
      partition_iterator end_partition(const size_type i)
      {
        check_for_empty("end_partition");
        return (i < m_Buckets.size()) ? partition_iterator{m_Buckets[i].end(), i} : partition_iterator{m_Buckets.back().end(), npos};
      }

      [[nodiscard]]
      const_partition_iterator begin_partition(const size_type i) const
      {
        check_for_empty("begin_partition");
        return begin_partition_raw(i);
      }

      [[nodiscard]]
      const_partition_iterator end_partition(const size_type i) const
      {
        check_for_empty("end_partition");
        return end_partition_raw(i);
      }

      [[nodiscard]]
      reverse_partition_iterator rbegin_partition(const size_type i)
      {
        check_for_empty("rbegin_partition");
        return (i < m_Buckets.size()) ? reverse_partition_iterator{m_Buckets[i].rbegin(), i} : reverse_partition_iterator{m_Buckets.front().rend(), npos};
      }

      [[nodiscard]]
      reverse_partition_iterator rend_partition(const size_type i)
      {
        check_for_empty("rend_partition");
        return (i < m_Buckets.size()) ? reverse_partition_iterator{m_Buckets[i].rend(), i} : reverse_partition_iterator{m_Buckets.front().rend(), npos};
      }

      [[nodiscard]]
      const_reverse_partition_iterator rbegin_partition(const size_type i) const
      {
        check_for_empty("rbegin_partition");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator{m_Buckets[i].crbegin(), i} : const_reverse_partition_iterator{m_Buckets.front().crend(), npos};

      }

      [[nodiscard]]
      const_reverse_partition_iterator rend_partition(const size_type i) const
      {
        check_for_empty("rend_partition");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator{m_Buckets[i].crend(), i} : const_reverse_partition_iterator{m_Buckets.front().crend(), npos};
      }

      [[nodiscard]]
      const_partition_iterator cbegin_partition(const size_type i) const
      {
        return begin_partition(i);
      }

      [[nodiscard]]
      const_partition_iterator cend_partition(const size_type i) const
      {
        return end_partition(i);
      }

      [[nodiscard]]
      const_reverse_partition_iterator crbegin_partition(const size_type i) const
      {
        return rbegin_partition(i);
      }

      [[nodiscard]]
      const_reverse_partition_iterator crend_partition(const size_type i) const
      {
        return rend_partition(i);
      }

      [[nodiscard]]
      partition_range partition(const size_type i)
      {
        check_for_empty("partition");

        return (i < m_Buckets.size()) ? partition_range{partition_iterator{m_Buckets[i].begin(), i}, partition_iterator{m_Buckets[i].end(), i}}
                                      : partition_range{partition_iterator{m_Buckets.back().end(), npos}, partition_iterator{m_Buckets.back().end(), npos}};
      }

      [[nodiscard]]
      const_partition_range partition(const size_type i) const
      {
        check_for_empty("partition");

        return (i < m_Buckets.size()) ? const_partition_range{const_partition_iterator{m_Buckets[i].begin(), i}, const_partition_iterator{m_Buckets[i].end(), i}}
                                      : const_partition_range{const_partition_iterator{m_Buckets.back().end(), npos}, const_partition_iterator{m_Buckets.back().end(), npos}};
      }

      [[nodiscard]]
      const_partition_range cpartition(const size_type i) const { return partition(i); }

      [[nodiscard]]
      const_partition_iterator operator[](const size_type i) const { return cbegin_partition(i); }

      [[nodiscard]]
      partition_iterator operator[](const size_type i) { return begin_partition(i); }

      void reset(const allocator_type& allocator) noexcept
        requires (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value)
      {
        const container_type buckets(allocator);
        m_Buckets = buckets;
      }

      [[nodiscard]]
      friend bool operator==(const bucketed_sequence& lhs, const bucketed_sequence& rhs) noexcept = default;
    private:
      container_type m_Buckets;

      void check_range(std::string_view method, const size_type index) const
      {
        if(index >= m_Buckets.size())
        {
          throw std::out_of_range{std::string{"bucketed_sequence::"}.append(method).append("index ").append(std::to_string(index)).append(" out of range")};
        }
      }

      void check_range(std::string_view method, const size_type index, const size_type pos) const
      {
        check_range(method, index);
        const auto bucketSize{m_Buckets[index].size()};
        if(pos > bucketSize)
        {
          throw std::out_of_range{std::string{"bucketed_sequence::"}.append(method).append("pos ").append(std::to_string(pos)).append(" out of range")};
        }
      }

      void check_for_empty(std::string_view method) const
      {
          if(m_Buckets.empty()) throw std::out_of_range{std::string{"bucketed_sequence::"}.append(method).append(": no buckets\n")};
      }

      [[nodiscard]]
      const_partition_iterator begin_partition_raw(const size_type i) const
      {
        return (i < m_Buckets.size()) ? const_partition_iterator{m_Buckets[i].cbegin(), i} : const_partition_iterator{m_Buckets.back().cend(), npos};
      }

      [[nodiscard]]
      const_partition_iterator end_partition_raw(const size_type i) const
      {
        return (i < m_Buckets.size()) ? const_partition_iterator{m_Buckets[i].cend(), i} : const_partition_iterator{m_Buckets.back().cend(), npos};
      }
    };

    //===================================Contiguous storage===================================//

    /*! \brief Base class for partitioned sequences where data is contiguous across all partitions.
     */

    template<class T, class Container, class Partitions>
    class partitioned_sequence_base
    {
    public:
      using value_type      = T;
      using container_type  = Container;
      using partitions_type = Partitions;
      using index_type      = typename partitions_type::value_type;
      using size_type       = std::common_type_t<index_type, typename container_type::size_type>;

      using partition_iterator               = data_structures::partition_iterator<container_type, index_type>;
      using const_partition_iterator         = data_structures::const_partition_iterator<container_type, index_type>;
      using reverse_partition_iterator       = data_structures::reverse_partition_iterator<container_type, index_type>;
      using const_reverse_partition_iterator = data_structures::const_reverse_partition_iterator<container_type, index_type>;
      using partition_range                  = std::ranges::subrange<partition_iterator>;
      using const_partition_range            = std::ranges::subrange<const_partition_iterator>;

      constexpr partitioned_sequence_base() = default;

      constexpr partitioned_sequence_base(const partitioned_sequence_base&) = default;

      [[nodiscard]]
      constexpr bool empty() const noexcept { return m_Data.empty(); }

      [[nodiscard]]
      constexpr auto size() const noexcept { return m_Data.size(); }

      [[nodiscard]]
      constexpr size_type size_of_partition(index_type i) const
      {
        return static_cast<size_type>(std::ranges::distance(partition(i)));
      }

      [[nodiscard]]
      constexpr auto num_partitions() const noexcept { return m_Partitions.size(); }

      [[nodiscard]]
      constexpr partition_iterator begin_partition(const index_type i) noexcept
      {
        return get_begin_iterator<partition_iterator>(i, m_Data.begin());
      }

      [[nodiscard]]
      constexpr partition_iterator end_partition(const index_type i) noexcept
      {
        return get_end_iterator<partition_iterator>(i, m_Data.begin());
      }

      [[nodiscard]]
      constexpr const_partition_iterator begin_partition(const index_type i) const noexcept
      {
        return get_begin_iterator<const_partition_iterator>(i, m_Data.cbegin());
      }

      [[nodiscard]]
      constexpr const_partition_iterator end_partition(const index_type i) const noexcept
      {
        return get_end_iterator<const_partition_iterator>(i, m_Data.cbegin());
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
        return get_end_iterator<reverse_partition_iterator>(i, m_Data.rend());
      }

      [[nodiscard]]
      constexpr reverse_partition_iterator rend_partition(const index_type i) noexcept
      {
        return get_begin_iterator<reverse_partition_iterator>(i, m_Data.rend());
      }

      [[nodiscard]]
      constexpr const_reverse_partition_iterator rbegin_partition(const index_type i) const noexcept
      {
        return get_end_iterator<const_reverse_partition_iterator>(i, m_Data.crend());
      }

      [[nodiscard]]
      constexpr const_reverse_partition_iterator rend_partition(const index_type i) const noexcept
      {
        return get_begin_iterator<const_reverse_partition_iterator>(i, m_Data.crend());
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
      constexpr partition_range partition(const index_type i) noexcept { return {begin_partition(i), end_partition(i)}; }

      [[nodiscard]]
      constexpr const_partition_range partition(const index_type i) const noexcept { return {begin_partition(i), end_partition(i)}; }

      [[nodiscard]]
      constexpr const_partition_range cpartition(const index_type i) const noexcept { return partition(i); }

      [[nodiscard]]
      constexpr const_partition_iterator operator[](const index_type i) const noexcept { return cbegin_partition(i); }

      [[nodiscard]]
      constexpr partition_iterator operator[](const index_type i) noexcept { return begin_partition(i); }

      constexpr void swap_partitions(index_type i, index_type j)
      {
        if((i < num_partitions()) && (j < num_partitions()))
        {
          if(i != j)
          {
            if(i > j) std::ranges::swap(i, j);

            const auto len_i{std::ranges::distance(partition(i))};
            const auto len_j{std::ranges::distance(partition(j))};

            auto begin_i{begin_partition(i).base_iterator()}, begin_j{begin_partition(j).base_iterator()};
            auto end_i{end_partition(i).base_iterator()}, end_j{end_partition(j).base_iterator()};
            for(auto iter_i{begin_i}, iter_j{begin_j}; (iter_i != end_i) && (iter_j != end_j); ++iter_i, ++iter_j)
            {
              std::ranges::iter_swap(iter_i, iter_j);
            }

            if(len_i > len_j)
            {
              std::ranges::rotate(begin_i + len_j, begin_i + len_i, end_j);
            }
            else if(len_j > len_i)
            {
              std::ranges::rotate(begin_i + len_i, begin_j + len_i, end_j);
            }

            if(len_i != len_j)
            {
              m_Partitions.mutate(maths::unsafe_t{},
                                  m_Partitions.begin() + i,
                                  m_Partitions.begin() + j,
                                  [len_i, len_j](const index_type index) -> index_type { return static_cast<index_type>(index + len_j - len_i);});
            }
          }
        }
      }

      template<alloc Allocator, alloc PartitionsAllocator>
        requires (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value
      && std::allocator_traits<PartitionsAllocator>::propagate_on_container_copy_assignment::value)
        void reset(const Allocator& allocator, const PartitionsAllocator& partitionsAllocator) noexcept
      {
        const partitions_type partitions(partitionsAllocator);
        m_Partitions = partitions;

        const container_type container{allocator};
        m_Data = container;
      }

      [[nodiscard]]
      friend constexpr bool operator==(const partitioned_sequence_base&, const partitioned_sequence_base&) noexcept = default;
    protected:
      explicit constexpr partitioned_sequence_base(const std::pair<partitions_type, container_type>& data)
        : m_Partitions{data.first}
        , m_Data{data.second}
      {}

      constexpr partitioned_sequence_base(partitioned_sequence_base&&) noexcept = default;

      constexpr partitioned_sequence_base& operator=(partitioned_sequence_base&&) noexcept = default;

      constexpr partitioned_sequence_base& operator=(const partitioned_sequence_base&) = default;

      ~partitioned_sequence_base() = default;

      void swap(partitioned_sequence_base& other)
        noexcept(noexcept(std::ranges::swap(this->m_Partitions, other.m_Partitions)) && noexcept(std::ranges::swap(this->m_Data, other.m_Data)))
      {
        std::ranges::swap(m_Partitions, other.m_Partitions);
        std::ranges::swap(m_Data, other.m_Data);
      }

      [[nodiscard]]
      auto get_allocator() const
      {
        return m_Data.get_allocator();
      }

      [[nodiscard]]
      auto get_partitions_allocator() const
      {
        return m_Partitions.get_allocator();
      }

      template<alloc Allocator>
      constexpr explicit partitioned_sequence_base(const Allocator& allocator) noexcept
        : m_Data(allocator)
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(const Allocator& allocator, const PartitionsAllocator& partitionsAllocator) noexcept
        : m_Partitions(partitionsAllocator)
        , m_Data(allocator)
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(std::initializer_list<std::initializer_list<T>> list, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions(partitionsAllocator)
        , m_Data(allocator)
      {
        init(list);
      }

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(const partitioned_sequence_base& other, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions{other.m_Partitions, partitionsAllocator}
        , m_Data(copy(other.m_Data, allocator))
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(partitioned_sequence_base&& in, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions{std::move(in.m_Partitions), partitionsAllocator}
        , m_Data{std::move(in.m_Data), allocator}
      {}

      void add_slot()
      {
        m_Partitions.push_back(m_Data.size());
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
            m_Data.erase(m_Data.begin() + offset, m_Data.begin() + m_Partitions[n]);
          }
          m_Partitions.erase(m_Partitions.begin() + n);
          m_Partitions.mutate(maths::unsafe_t{}, m_Partitions.begin() + n, m_Partitions.end(), [erased](const auto index){ return index - erased; });
        }
      }

      void reserve(const size_type size)
      {
        m_Data.reserve(size);
      }

      [[nodiscard]]
      index_type capacity() const noexcept
      {
        return m_Data.capacity();
      }

      void reserve_partitions(const size_type numPartitions)
      {
        m_Partitions.reserve(numPartitions);
      }

      [[nodiscard]]
      index_type num_partitions_capacity() const noexcept
      {
        return m_Partitions.capacity();
      }

      void shrink_to_fit()
      {
        m_Partitions.shrink_to_fit();
        m_Data.shrink_to_fit();
      }

      void clear() noexcept
      {
        m_Partitions.clear();
        m_Data.clear();
      }

      template<class... Args>
      void push_back_to_partition(const index_type index, Args&&... args)
      {
        check_range("push_back_to_partition", index);

        auto iter{m_Data.end()};
        if(index == m_Partitions.size() - 1)
        {
          m_Data.emplace_back(std::forward<Args>(args)...);
          iter = --m_Data.end();
        }
        else
        {
          iter = m_Data.emplace(m_Data.begin() + m_Partitions[index], std::forward<Args>(args)...);
        }

        increment_partition_indices(index);
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, Args&&... args)
      {
        const auto source{pos.partition_index()};
        check_range("insert_to_partition", source);

        auto iter{m_Data.emplace(pos.base_iterator(), std::forward<Args>(args)...)};
        increment_partition_indices(source);

        return partition_iterator{iter, source};
      }

      template<class... Args>
      partition_iterator insert_to_partition(const size_type index, const size_type pos, Args&&... args)
      {
        return insert_to_partition(std::ranges::next(cbegin_partition(index), pos, cend_partition(index)), std::forward<Args>(args)...);
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto partition{iter.partition_index()};
        if(const auto n{num_partitions()}; partition >= n)
        {
          return end_partition(n);
        }
        else if(iter == cend_partition(partition))
        {
          return end_partition(partition);
        }

        const auto next{m_Data.erase(iter.base_iterator())};
        decrement_partition_indices(partition, num_partitions(), 1);

        return {next, iter.partition_index()};
      }

      partition_iterator erase_from_partition(const_partition_iterator first, const_partition_iterator last)
      {
        const auto firstPartition{first.partition_index()}, lastPartition{last.partition_index()};
        if(const auto n{num_partitions()}; (firstPartition >= n) && (lastPartition >= n))
        {
          return end_partition(n);
        }
        else if(lastPartition == firstPartition)
        {
          if(const auto numErased{std::ranges::distance(first, last)}; numErased > 0)
          {
            const auto next{m_Data.erase(first.base_iterator(), last.base_iterator())};
            decrement_partition_indices(firstPartition, num_partitions(), numErased);

            return {next, firstPartition};
          }

          return std::ranges::next(begin_partition(firstPartition), std::ranges::distance(cbegin_partition(firstPartition), first));
        }

        throw std::domain_error{std::string{"partitioned_sequence::erase - Invalid range specified by iterators belonging to partitions ["}
          .append(std::to_string(firstPartition)).append(", ").append(std::to_string(lastPartition)).append("]")};
      }

      partition_iterator erase_from_partition(const index_type index, const size_type pos)
      {
        return erase_from_partition(std::ranges::next(cbegin_partition(index), pos, cend_partition(index)));
      }
    private:
      constexpr static index_type npos{partition_iterator::npos};

      SEQUOIA_NO_UNIQUE_ADDRESS partitions_type m_Partitions;
      container_type m_Data;

      partitioned_sequence_base(std::initializer_list<std::initializer_list<T>> list)
      {
        init(list);
      }

      void init(std::initializer_list<std::initializer_list<T>> list)
      {
        m_Partitions.reserve(list.size());
        m_Data.reserve(std::accumulate(list.begin(), list.end(), std::size_t{}, [](std::size_t n, auto l) { return n += l.size(); }));
        for(auto iter{list.begin()}; iter != list.end(); ++iter)
        {
          add_slot();
          const auto dist{std::ranges::distance(list.begin(), iter)};
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
      }

      template<alloc Allocator>
      [[nodiscard]]
      static container_type copy(const container_type& other, const Allocator& a)
      {
        container_type container(std::allocator_traits<Allocator>::select_on_container_copy_construction(a));

        container.reserve(other.size());
        for(const auto& elt : other)
        {
          container.push_back(elt);
        }

        return container;
      }

      void increment_partition_indices(const size_type first) noexcept
      {
        m_Partitions.mutate(maths::unsafe_t{},
                            std::ranges::next(m_Partitions.begin(), first, m_Partitions.end()),
                            m_Partitions.end(),
                            [](index_type index){ return ++index; });
      }

      void decrement_partition_indices(const size_type first, const size_type last, const index_type num) noexcept
      {
        m_Partitions.mutate(maths::unsafe_t{},
                            std::ranges::next(m_Partitions.begin(), first, m_Partitions.end()),
                            std::ranges::next(m_Partitions.begin(), last, m_Partitions.end()),
                            [num](index_type index){ return index -= num; });
      }

      void check_range(std::string_view method, const size_type index) const
      {
        if(index >= m_Partitions.size())
        {
          throw std::out_of_range{std::string{"partition_sequence::"}.append(method).append("index ").append(std::to_string(index)).append(" out of range")};
        }
      }

      void check_range(std::string_view method, const size_type index, const index_type pos) const
      {
        check_range(method, index);
        const index_type maxPos{index ? m_Partitions[index] - m_Partitions[index - 1] : m_Partitions[index]};
        if(pos > maxPos)
        {
          throw std::out_of_range{std::string{"partition_sequence::"}.append(method).append("pos ").append(std::to_string(pos)).append(" out of range")};
        }
      }

      template<class PartitionIterator, std::input_or_output_iterator Iterator>
      [[nodiscard]]
      constexpr PartitionIterator get_begin_iterator(const index_type i, Iterator iter) const noexcept
      {
        const index_type index{i >= m_Partitions.size() ? npos : i};
        const auto offset{(i > 0 && i < m_Partitions.size()) ? m_Partitions[i-1] : index_type{}};
        return PartitionIterator::reversed() ? PartitionIterator{iter, index} - offset : PartitionIterator{iter, index} + offset;
      }

      template<class PartitionIterator, std::input_or_output_iterator Iterator>
      [[nodiscard]]
      constexpr PartitionIterator get_end_iterator(const index_type i, Iterator iter) const
      {
        index_type index{PartitionIterator::reversed() ? index_type{} : npos};
        index_type offset{
          [sz{m_Data.size()}] () {
            if (sz > std::numeric_limits<index_type>::max())
              throw std::out_of_range{"Partition offset out of range"};
            return static_cast<index_type>(sz);
          }()
        };

        if(i < m_Partitions.size())
        {
          index = i;
          offset = m_Partitions[i];
        }

        return PartitionIterator::reversed() ? PartitionIterator{iter, index} - offset : PartitionIterator{iter, index} + offset;
      }
    };

    /*! \class
    
     */

    template<class T, class Container=std::vector<T>, class Partitions=maths::monotonic_sequence<std::size_t, std::ranges::greater>>
    class partitioned_sequence : public partitioned_sequence_base<T, Container, Partitions>
    {
    private:
      using base_t = partitioned_sequence_base<T, Container, Partitions>;
    public:
      using container_type            = typename partitioned_sequence_base<T, Container, Partitions>::container_type;
      using partitions_type           = typename partitioned_sequence_base<T, Container, Partitions>::partitions_type;
      using allocator_type            = typename container_type::allocator_type;
      using partitions_allocator_type = typename partitions_type::allocator_type;

      partitioned_sequence() = default;

      partitioned_sequence(const allocator_type& allocator, const partitions_allocator_type& partitionAllocator) noexcept
        : partitioned_sequence_base<T, Container, Partitions>(allocator, partitionAllocator)
      {}

      partitioned_sequence(std::initializer_list<std::initializer_list<T>> list, const allocator_type& allocator=allocator_type{}, const partitions_allocator_type& partitionAllocator=partitions_allocator_type{})
        : partitioned_sequence_base<T, Container, Partitions>(list, allocator, partitionAllocator)
      {}

      partitioned_sequence(const partitioned_sequence&) = default;

      partitioned_sequence(const partitioned_sequence& s, const allocator_type& allocator, const partitions_allocator_type& partitionAllocator)
        : partitioned_sequence_base<T, Container, Partitions>(s, allocator, partitionAllocator)
      {}

      partitioned_sequence(partitioned_sequence&&) noexcept = default;

      partitioned_sequence(partitioned_sequence&& s, const allocator_type& allocator, const partitions_allocator_type& partitionAllocator)
        : partitioned_sequence_base<T, Container, Partitions>(std::move(s), allocator, partitionAllocator)
      {}

      ~partitioned_sequence() = default;

      partitioned_sequence& operator=(const partitioned_sequence&)     = default;
      partitioned_sequence& operator=(partitioned_sequence&&) noexcept = default;

      using base_t::swap;

      friend void swap(partitioned_sequence& lhs, partitioned_sequence& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
      {
        lhs.swap(rhs);
      }

      using base_t::add_slot;
      using base_t::insert_slot;
      using base_t::erase_slot;

      using base_t::reserve;
      using base_t::capacity;
      using base_t::reserve_partitions;
      using base_t::num_partitions_capacity;
      using base_t::shrink_to_fit;
      using base_t::get_allocator;
      using base_t::get_partitions_allocator;

      using base_t::clear;
      using base_t::push_back_to_partition;
      using base_t::insert_to_partition;
      using base_t::erase_from_partition;
    };

    template<class T>
    struct static_partitions_maker;

    template<std::integral IndexType, std::size_t Npartitions>
    struct static_partitions_maker<maths::static_monotonic_sequence<IndexType, Npartitions, std::ranges::greater>>
    {
      using partitions_type = maths::static_monotonic_sequence<IndexType, Npartitions, std::ranges::greater>;

      constexpr static std::size_t num_partitions_v{Npartitions};

      template<class T>
      [[nodiscard]]
      constexpr static partitions_type make_partitions(std::initializer_list<std::initializer_list<T>> list)
      {
        constexpr auto upper{std::numeric_limits<IndexType>::max()};
        const auto fn{
          [](std::initializer_list<T> l) -> IndexType {
            if(l.size() > upper)
              throw std::out_of_range{"Partition size out of bounds"};

            return static_cast<IndexType>(l.size());
          }
        };

        auto sizes{utilities::to_array<IndexType, num_partitions_v>(list, fn)};
        if(!sizes.empty())
        {
          for(auto iter{sizes.begin() + 1}; iter != sizes.end(); ++iter)
          {
            const auto prev{*(iter - 1)};
            auto& curr{*iter};
            if(upper - prev < curr)
              throw std::out_of_range{"Partition index out of bounds"};

            curr += prev;
          }
        }

        return sizes;
      }

    };

    template<class T, std::size_t Npartitions, std::size_t Nelements, class Partitions=maths::static_monotonic_sequence<std::size_t, Npartitions, std::ranges::greater>>
    class static_partitioned_sequence :
      public partitioned_sequence_base<T, std::array<T, Nelements>, Partitions>
    {
      using base_t = partitioned_sequence_base<T, std::array<T, Nelements>, Partitions>;
    public:
      using container_type  = typename base_t::container_type;
      using partitions_type = typename base_t::partitions_type;
      using size_type       = typename base_t::size_type;
      using index_type      = typename base_t::index_type;

      constexpr static std::size_t num_partitions_v{Npartitions};
      constexpr static std::size_t num_elements_v{Nelements};

      constexpr static_partitioned_sequence() = default;

      constexpr static_partitioned_sequence(std::initializer_list<std::initializer_list<T>> list)
        : base_t{fill(std::make_index_sequence<num_elements_v>(), list)}
      {}

      constexpr static_partitioned_sequence(const static_partitioned_sequence&)     = default;
      constexpr static_partitioned_sequence(static_partitioned_sequence&&) noexcept = default;

      constexpr static_partitioned_sequence& operator=(const static_partitioned_sequence&)     = default;
      constexpr static_partitioned_sequence& operator=(static_partitioned_sequence&&) noexcept = default;

      using base_t::swap;

      friend void swap(static_partitioned_sequence& lhs, static_partitioned_sequence& rhs)
        noexcept(noexcept(lhs.swap(rhs)))
      {
        lhs.swap(rhs);
      }
    private:
      template<size_type... Inds>
      [[nodiscard]]
      constexpr static std::pair<partitions_type, container_type> fill(std::index_sequence<Inds...>, std::initializer_list<std::initializer_list<T>> list)
      {
        if(list.size() != num_partitions_v)
          throw std::logic_error{std::string{"Inconsistent number of elements supplied by initializer lists: expected "}
              .append(std::to_string(num_partitions_v)).append(" but got ").append(std::to_string(list.size()))};

        auto partitions{static_partitions_maker<partitions_type>::make_partitions(list)};

        const size_type total{std::accumulate(list.begin(), list.end(), size_type{}, [](size_type val, std::initializer_list<T> l){ return val + l.size(); })};

        if(total != num_elements_v)
          throw std::logic_error{std::string{"Inconsistent number of elements supplied by initializer lists: expected "}
              .append(std::to_string(num_elements_v)).append(" but got ").append(std::to_string(total))};

        if constexpr(sizeof...(Inds) > 0)
        {
          index_type partitionIndex{};
          return {partitions, container_type{make_element(Inds, list, partitions, partitionIndex)...}};
        }
        else
        {
          return {partitions, container_type{}};
        }
      }


      [[nodiscard]]
      constexpr static T make_element(size_type i, std::initializer_list<std::initializer_list<T>> list, const partitions_type& partitions, index_type& partitionIndex)
      {
        auto boundary{partitions[partitionIndex]};
        while(i == boundary)
        {
          boundary = partitions[++partitionIndex];
        }

        auto index{partitionIndex ? i - partitions[partitionIndex - 1] : i};

        return T{*((list.begin() + partitionIndex)->begin() + index)};
      }
    };
  }
}
