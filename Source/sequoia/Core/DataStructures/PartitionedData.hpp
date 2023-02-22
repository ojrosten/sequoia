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
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/Core/ContainerUtilities/ArrayUtilities.hpp"
#include "sequoia/Core/ContainerUtilities/Iterator.hpp"
#include "sequoia/Algorithms/Algorithms.hpp"
#include "sequoia/Maths/Sequences/MonotonicSequence.hpp"
#include "sequoia/Core/ContainerUtilities/AssignmentUtilities.hpp"
#include "sequoia/PlatformSpecific/Preprocessor.hpp"

#include <string>
#include <numeric>
#include <stdexcept>

namespace sequoia
{
  namespace data_structures
  {
    //===================================A Custom Iterator===================================//

    template<class Traits, class Handler, std::integral IndexType>
      requires object::handler<Handler>
    using partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, Handler, partition_impl::mutable_reference, false>::iterator,
          partition_impl::dereference_policy<Handler, partition_impl::mutable_reference, partition_impl::partition_index_policy<false, IndexType>>
        >;

    template<class Traits, class Handler, std::integral IndexType>
      requires object::handler<Handler>
    using const_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, Handler, partition_impl::const_reference, false>::iterator,
        partition_impl::dereference_policy<Handler, partition_impl::const_reference, partition_impl::partition_index_policy<false, IndexType>>
      >;

    template<class Traits, class Handler, std::integral IndexType>
      requires object::handler<Handler>
    using reverse_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, Handler, partition_impl::mutable_reference, true>::iterator,
        partition_impl::dereference_policy<Handler, partition_impl::mutable_reference, partition_impl::partition_index_policy<true, IndexType>>
      >;

    template<class Traits, class Handler, std::integral IndexType>
      requires object::handler<Handler>
    using const_reverse_partition_iterator
      = utilities::iterator<
          typename partition_impl::partition_iterator_generator<Traits, Handler, partition_impl::const_reference, true>::iterator,
          partition_impl::dereference_policy<Handler, partition_impl::const_reference, partition_impl::partition_index_policy<true, IndexType>>
      >;

    //===================================Storage using buckets===================================//

    template<class T, class Handler>
      requires object::handler<Handler>
    struct bucketed_sequence_traits
    {
      constexpr static bool throw_on_range_error{true};

      template<class S>
      using container_type = std::vector<S>;

      template<class S>
      using buckets_type = std::vector<std::vector<S>>;
    };

    /*! \brief Storage for partitioned data such that data within each partition is contiguous.
     */

    template<class T, class Handler=object::by_value<T>, class Traits=bucketed_sequence_traits<T, Handler>>
      requires object::handler<Handler>
    class bucketed_sequence
    {
      using held_type    = typename Handler::product_type;
      using storage_type = typename Traits::template buckets_type<held_type>;
    public:
      using value_type     = T;
      using size_type      = typename storage_type::size_type;
      using index_type     = size_type;
      using allocator_type = typename storage_type::allocator_type;
      using handler_type   = Handler;

      using partition_iterator               = data_structures::partition_iterator<Traits, Handler, size_type>;
      using const_partition_iterator         = data_structures::const_partition_iterator<Traits, Handler, size_type>;
      using reverse_partition_iterator       = data_structures::reverse_partition_iterator<Traits, Handler, size_type>;
      using const_reverse_partition_iterator = data_structures::const_reverse_partition_iterator<Traits, Handler, size_type>;

      constexpr static bool throw_on_range_error{Traits::throw_on_range_error};

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
          const auto dist{std::distance(list.begin(), iter)};
          m_Buckets[dist].reserve(iter->size());
          for(const auto& element : (*iter))
          {
            push_back_to_partition(dist, element);
          }
        }
      }

      bucketed_sequence(const bucketed_sequence& in)
        : bucketed_sequence(partition_impl::copy_constant<directCopy>{}, in)
      {}

     bucketed_sequence(const bucketed_sequence& in, const allocator_type& allocator)
       : m_Buckets(std::allocator_traits<allocator_type>::select_on_container_copy_construction(allocator))
      {
        m_Buckets.reserve(in.m_Buckets.size());

        partition_impl::data_duplicator<Handler> duplicator;
        for(auto i{in.m_Buckets.cbegin()}; i != in.m_Buckets.cend(); ++i)
        {
          const auto& bucket{*i};
          const auto dist{std::distance(in.m_Buckets.cbegin(), i)};
          add_slot();
          m_Buckets.back().reserve(in.m_Buckets[dist].size());
          for(const auto& elt : bucket)
          {
            m_Buckets.back().push_back(duplicator.duplicate(elt));
          }
        }
      }

      bucketed_sequence(bucketed_sequence&& in) noexcept = default;

      bucketed_sequence(bucketed_sequence&& in, const allocator_type& allocator)
        : m_Buckets(std::move(in).m_Buckets, allocator)
      {}

      bucketed_sequence& operator=(bucketed_sequence&& in) = default;

      bucketed_sequence& operator=(const bucketed_sequence& in)
      {
        if(&in != this)
        {
          auto allocGetter{
            [](const  bucketed_sequence& s){
              return s.get_allocator();
            }
          };
          assignment_helper::assign(*this, in, allocGetter);
        }

        return *this;
      }

      void swap(bucketed_sequence& other)
        noexcept(noexcept(std::swap(this->m_Buckets, other.m_Buckets)))
      {
        std::swap(m_Buckets, other.m_Buckets);
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
        return static_cast<size_type>(std::distance(begin_partition(i), end_partition(i)));
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
          throw std::out_of_range("bucketed_sequence::swap_partitions - index out of range");
        }
      }

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

        m_Buckets[index].push_back(Handler::producer_type::make(std::forward<Args>(args)...));
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
          if(!m_Buckets.size()) throw std::out_of_range("bucketed_sequence: no partitions into which to insert");
        }

        const auto soure{pos.partition_index()};
        auto iter{m_Buckets[soure].insert(pos.base_iterator(), Handler::producer_type::make(std::forward<Args>(args)...))};

        return partition_iterator{iter, soure};
      }

      template<class... Args>
      partition_iterator insert_to_partition(const_partition_iterator pos, const_partition_iterator setFromIter)
      {
        if constexpr(throw_on_range_error)
        {
          if(!m_Buckets.size()) throw std::out_of_range("bucketed_sequence: no partitions into which to insert");
        }

        const auto soure{pos.partition_index()};
        auto iter{m_Buckets[soure].insert(pos.base_iterator(), *(setFromIter.base_iterator()))};

        return partition_iterator{iter, soure};
      }

      partition_iterator erase_from_partition(const size_type index, const size_type pos)
      {
        auto iter{begin_partition(index)};
        auto next{end_partition(index).base_iterator()};
        if(iter != end_partition(index) && pos < static_cast<size_type>(distance(iter, end_partition(index))))
        {
          next = m_Buckets[index].erase((iter + pos).base_iterator());
        }

        return {next, index};
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto partition{iter.partition_index()};
        if (iter == cend_partition(partition)) return { m_Buckets[partition].end(), partition };

        const auto next{m_Buckets[partition].erase(iter.base_iterator())};
        return {next, partition};
      }

      partition_iterator begin_partition(const size_type i)
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? partition_iterator{m_Buckets[i].begin(), i} : partition_iterator{m_Buckets.back().end(), npos};
      }

      partition_iterator end_partition(const size_type i)
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? partition_iterator{m_Buckets[i].end(), i} : partition_iterator{m_Buckets.back().end(), npos};
      }

      const_partition_iterator begin_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_partition_iterator{m_Buckets[i].cbegin(), i} : const_partition_iterator{m_Buckets.back().cend(), npos};
      }

      const_partition_iterator end_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_partition_iterator{m_Buckets[i].cend(), i} : const_partition_iterator{m_Buckets.back().cend(), npos};
      }

      reverse_partition_iterator rbegin_partition(const size_type i)
      {
        if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? reverse_partition_iterator{m_Buckets[i].rbegin(), i} : reverse_partition_iterator{m_Buckets.front().rend(), npos};
      }

      reverse_partition_iterator rend_partition(const size_type i)
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? reverse_partition_iterator{m_Buckets[i].rend(), i} : reverse_partition_iterator{m_Buckets.front().rend(), npos};
      }

      const_reverse_partition_iterator rbegin_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::begin_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator{m_Buckets[i].crbegin(), i} : const_reverse_partition_iterator{m_Buckets.front().crend(), npos};

      }

      const_reverse_partition_iterator rend_partition(const size_type i) const
      {
        if constexpr(throw_on_range_error) if(m_Buckets.empty()) throw std::out_of_range("bucketed_sequence::end_partition: no buckets!\n");
        return (i < m_Buckets.size()) ? const_reverse_partition_iterator{m_Buckets[i].crend(), i} : const_reverse_partition_iterator{m_Buckets.front().crend(), npos};
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

      void reset(const allocator_type& allocator) noexcept
        requires (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value)
      {
        const storage_type buckets(allocator);
        m_Buckets = buckets;
      }

      [[nodiscard]]
      friend bool operator==(const bucketed_sequence& lhs, const bucketed_sequence& rhs) noexcept
      {
        if constexpr(std::is_same_v<Handler, object::by_value<T>>)
        {
          return lhs.m_Buckets == rhs.m_Buckets;
        }
        else
        {
          return isomorphic(lhs, rhs);
        }
      }
    private:
      constexpr static auto npos{partition_iterator::npos};
      constexpr static bool directCopy{partition_impl::direct_copy_v<Handler, T>};

      storage_type m_Buckets;

      bucketed_sequence(partition_impl::direct_copy_type, const bucketed_sequence& in)
        : m_Buckets{in.m_Buckets}
      {}

      bucketed_sequence(partition_impl::indirect_copy_type, const bucketed_sequence& in)
        : bucketed_sequence(in, in.m_Buckets.get_allocator())
      {}

      void check_range(const size_type index) const
      {
        if(index >= m_Buckets.size())
        {
          throw std::out_of_range("bucketed_sequence::index " + std::to_string(index) + " out of range!");
        }
      }

      void check_range(const size_type index, const size_type pos) const
      {
        check_range(index);
        const auto bucketSize{m_Buckets[index].size()};
        if(pos > bucketSize)
        {
          throw std::out_of_range("bucketed_sequence::partition " + std::to_string(index) + " pos " + std::to_string(pos) + " out of range");
        }
      }
    };

    //===================================Contiguous storage===================================//

    /*! \brief Base class for partitioned sequences where data is contiguous across all partitions.
     */

    template<class T, class Handler, class Traits>
      requires object::handler<Handler>
    class partitioned_sequence_base
    {
      friend struct sequoia::assignment_helper;
    public:
      using value_type          = T;
      using handler_type        = Handler;
      using traits_type         = Traits;

      using container_type      = typename partition_impl::storage_type_generator<Traits, Handler>::container_type;
      using size_type           = std::common_type_t<typename Traits::partition_index_type, typename container_type::size_type>;
      using index_type          = typename Traits::index_type;

      using partition_iterator               = data_structures::partition_iterator<Traits, Handler, index_type>;
      using const_partition_iterator         = data_structures::const_partition_iterator<Traits, Handler, index_type>;
      using reverse_partition_iterator       = data_structures::reverse_partition_iterator<Traits, Handler, index_type>;
      using const_reverse_partition_iterator = data_structures::const_reverse_partition_iterator<Traits, Handler, index_type>;

      constexpr static bool throw_on_range_error{Traits::throw_on_range_error};

      constexpr partitioned_sequence_base() = default;

      constexpr partitioned_sequence_base(std::initializer_list<std::initializer_list<T>> list)
        : partitioned_sequence_base(init_constant<staticStorage>{}, list)
      {}

      constexpr partitioned_sequence_base(const partitioned_sequence_base& in)
        : partitioned_sequence_base(partition_impl::copy_constant<directCopy>{}, in)
      {}

      [[nodiscard]]
      constexpr bool empty() const noexcept { return m_Storage.empty(); }

      [[nodiscard]]
      constexpr auto size() const noexcept { return m_Storage.size(); }

      [[nodiscard]]
      constexpr size_type size_of_partition(index_type i) const
      {
        return static_cast<size_type>(std::distance(begin_partition(i), end_partition(i)));
      }

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
            if(i > j) std::swap(i, j);

            const auto len_i{distance(begin_partition(i), end_partition(i))};
            const auto len_j{distance(begin_partition(j), end_partition(j))};

            auto begin_i{begin_partition(i).base_iterator()}, begin_j{begin_partition(j).base_iterator()};
            auto end_i{end_partition(i).base_iterator()}, end_j{end_partition(j).base_iterator()};
            for(auto iter_i{begin_i}, iter_j{begin_j}; (iter_i != end_i) && (iter_j != end_j); ++iter_i, ++iter_j)
            {
              std::iter_swap(iter_i, iter_j);
            }

            if(len_i > len_j)
            {
              std::rotate(begin_i + len_j, begin_i + len_i, end_j);
            }
            else if(len_j > len_i)
            {
              std::rotate(begin_i + len_i, begin_j + len_i, end_j);
            }

            if(len_i != len_j)
            {
              m_Partitions.mutate(maths::unsafe_t{}, m_Partitions.begin() + i, m_Partitions.begin() + j,
                                  [len_i, len_j](const auto index){ return index + len_j - len_i;});
            }
          }
        }
        else if constexpr(throw_on_range_error)
        {
          throw std::out_of_range("partitioned_sequence::swap_partitions - index out of range");
        }
      }

      template<alloc Allocator, alloc PartitionsAllocator>
        requires (   std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value
                  && std::allocator_traits<PartitionsAllocator>::propagate_on_container_copy_assignment::value)
      void reset(const Allocator& allocator, const PartitionsAllocator& partitionsAllocator) noexcept
      {
        const PartitionsType partitions(partitionsAllocator);
        m_Partitions = partitions;

        const container_type container{allocator};
        m_Storage = container;
      }

      [[nodiscard]]
      friend constexpr bool operator==(const partitioned_sequence_base& lhs, const partitioned_sequence_base& rhs) noexcept
      {
        if constexpr(std::is_same_v<Handler, object::by_value<T>>)
        {
          return (lhs.m_Storage == rhs.m_Storage) && (lhs.m_Partitions == rhs.m_Partitions);
        }
        else
        {
          return isomorphic(lhs, rhs);
        }
      }
    protected:
      constexpr partitioned_sequence_base(partitioned_sequence_base&&) noexcept = default;

      constexpr partitioned_sequence_base& operator=(partitioned_sequence_base&&) = default;

      constexpr partitioned_sequence_base& operator=(const partitioned_sequence_base& in)
      {
        if(&in != this)
        {
          auto allocGetter{
            []([[maybe_unused]] const partitioned_sequence_base& in) {
              if constexpr(has_allocator_type_v<container_type>)
              {
                return in.m_Storage.get_allocator();
              }
            }
          };

          auto partitionsAllocGetter{
            []([[maybe_unused]] const partitioned_sequence_base& in){
              if constexpr(has_allocator_type_v<PartitionsType>)
              {
                return in.m_Partitions.get_allocator();
              }
            }
          };

          assignment_helper::assign(*this, in, allocGetter, partitionsAllocGetter);
        }

        return *this;
      }

      void swap(partitioned_sequence_base& other)
        // TO DO: Strictly speaking incorrect but will be fine when ranges::swap available
        noexcept(noexcept(std::swap(this->m_Partitions, other.m_Partitions)) && noexcept(std::swap(this->m_Storage, other.m_Storage)))
      {
        using std::swap;
        swap(m_Partitions, other.m_Partitions);
        swap(m_Storage, other.m_Storage);
      }

      auto get_allocator() const
      {
        return m_Storage.get_allocator();
      }

      auto get_partitions_allocator() const
      {
        return m_Partitions.get_allocator();
      }

      template<alloc Allocator>
      constexpr explicit partitioned_sequence_base(const Allocator& allocator) noexcept
        : m_Storage(allocator)
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(const Allocator& allocator, const PartitionsAllocator& partitionsAllocator) noexcept
        : m_Partitions(partitionsAllocator)
        , m_Storage(allocator)
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(std::initializer_list<std::initializer_list<T>> list, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions(partitionsAllocator)
        , m_Storage(allocator)
      {
        init(list);
      }

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(const partitioned_sequence_base& in, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : partitioned_sequence_base(partition_impl::copy_constant<directCopy>{}, in, allocator, partitionsAllocator)
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(partitioned_sequence_base&& in, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions{std::move(in.m_Partitions), partitionsAllocator}, m_Storage{std::move(in.m_Storage), allocator}
      {}

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
          m_Partitions.mutate(maths::unsafe_t{}, m_Partitions.begin() + n, m_Partitions.end(), [erased](const auto index){ return index - erased; });
        }
      }

      void reserve(const size_type size)
      {
        m_Storage.reserve(size);
      }

      [[nodiscard]]
      index_type capacity() const noexcept
      {
        return m_Storage.capacity();
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
            return Handler::producer_type::make(std::forward<decltype(a)>(a)...);
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
            return Handler::producer_type::make(std::forward<decltype(a)>(a)...);
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

        return {next, index};
      }

      partition_iterator erase_from_partition(const_partition_iterator iter)
      {
        const auto next{m_Storage.erase(iter.base_iterator())};
        const auto index{iter.partition_index()};
        decrement_partition_indices(index);

        return {next, iter.partition_index()};
      }
    private:
      template<bool Direct>
      struct init_constant : std::bool_constant<Direct>
      {};

      using static_init_type  = init_constant<true>;
      using dynamic_init_type = init_constant<false>;

      constexpr static bool staticStorage{Traits::static_storage_v};
      constexpr static bool directCopy{partition_impl::direct_copy_v<Handler, T>};

      using PartitionsType = typename Traits::partitions_type;
      constexpr static index_type npos{partition_iterator::npos};

      NO_UNIQUE_ADDRESS PartitionsType m_Partitions;
      container_type m_Storage;

      constexpr partitioned_sequence_base(static_init_type, std::initializer_list<std::initializer_list<T>> list)
        : m_Partitions{Traits::make_partitions(list)}
        , m_Storage{fill(std::make_index_sequence<Traits::num_elements_v>{}, list)}
      {}

      partitioned_sequence_base(dynamic_init_type, std::initializer_list<std::initializer_list<T>> list)
      {
        init(list);
      }

      constexpr partitioned_sequence_base(partition_impl::indirect_copy_type, const partitioned_sequence_base& in)
        : partitioned_sequence_base(partition_impl::indirect_copy_type{}, in, in.m_Storage.get_allocator())
      {}

      template<alloc Allocator>
      constexpr partitioned_sequence_base(partition_impl::indirect_copy_type, const partitioned_sequence_base& in, const Allocator& allocator)
        : m_Partitions{in.m_Partitions}
        , m_Storage(std::allocator_traits<Allocator>::select_on_container_copy_construction(allocator))
      {
        init(in.m_Storage);
      }

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(partition_impl::indirect_copy_type, const partitioned_sequence_base& in, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions{in.m_Partitions, partitionsAllocator}
        , m_Storage(std::allocator_traits<Allocator>::select_on_container_copy_construction(allocator))
      {
        init(in.m_Storage);
      }

      constexpr partitioned_sequence_base(partition_impl::direct_copy_type, const partitioned_sequence_base& in)
        : m_Partitions{in.m_Partitions}
        , m_Storage{in.m_Storage}
      {}

      template<alloc Allocator>
      constexpr partitioned_sequence_base(partition_impl::direct_copy_type, const partitioned_sequence_base& in, const Allocator& allocator)
        : m_Partitions{in.m_Partitions}
        , m_Storage{in.m_Storage, allocator}
      {}

      template<alloc Allocator, alloc PartitionsAllocator>
      constexpr partitioned_sequence_base(partition_impl::direct_copy_type, const partitioned_sequence_base& in, const Allocator& allocator, const PartitionsAllocator& partitionsAllocator)
        : m_Partitions{in.m_Partitions, partitionsAllocator}
        , m_Storage{in.m_Storage, allocator}
      {}

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
        partition_impl::data_duplicator<Handler> duplicator;
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

        return Handler::producer_type::make(*((list.begin() + partition)->begin() + index));
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

      void increment_partition_indices(const size_type first) noexcept
      {
        m_Partitions.mutate(maths::unsafe_t{}, m_Partitions.begin() + first, m_Partitions.end(), [](const auto index){ return index + 1; });
      }

      void decrement_partition_indices(const size_type first) noexcept
      {
        m_Partitions.mutate(maths::unsafe_t{}, m_Partitions.begin() + first, m_Partitions.end(), [](const auto index){ return index - 1; });
      }

      void check_range(const size_type index) const
      {
        if(index >= m_Partitions.size())
        {
          throw std::out_of_range("partitioned_sequence::index" + std::to_string(index) +  "out of range");
        }
      }

      void check_range(const size_type index, const index_type pos) const
      {
        check_range(index);
        const index_type maxPos{index ? m_Partitions[index] - m_Partitions[index - 1] : m_Partitions[index]};
        if(pos > maxPos)
        {
          throw std::out_of_range("partitioned_sequence::partition " + std::to_string(index) + " pos " + std::to_string(pos) + " out of range");
        }
      }

      void check_pos_validity(const index_type index, const index_type pos, const index_type expand)
      {
        const index_type maxPos{index ? m_Partitions[index] - m_Partitions[index - 1] : m_Partitions[index]};
        if(pos > maxPos + expand) throw std::out_of_range("partitioned_sequence::pos index out of range");
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
          if(!m_Partitions.size()) throw std::out_of_range("partitioned_sequence: no partitions into which to insert");
        }

        const auto soure{pos.partition_index()};
        auto iter{m_Storage.insert(pos.base_iterator(), maker(std::forward<Args>(args)...))};
        increment_partition_indices(soure);

        return partition_iterator{iter, soure};
      }

      template<class PartitionIterator, std::input_or_output_iterator Iterator>
      constexpr PartitionIterator get_begin_iterator(const index_type i, Iterator iter) const noexcept
      {
        const index_type index{i >= m_Partitions.size() ? npos : i};
        const auto offset{(i > 0 && i < m_Partitions.size()) ? m_Partitions[i-1] : index_type{}};
        return PartitionIterator::reversed() ? PartitionIterator{iter, index} - offset : PartitionIterator{iter, index} + offset;
      }

      template<class PartitionIterator, std::input_or_output_iterator Iterator>
      constexpr PartitionIterator get_end_iterator(const index_type i, Iterator iter) const
      {
        index_type index{PartitionIterator::reversed() ? index_type{} : npos};
        index_type offset{
          [sz{m_Storage.size()}] () {
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

    template<class T, class Handler>
      requires object::handler<Handler>
    struct partitioned_sequence_traits
    {
      constexpr static bool static_storage_v{false};
      constexpr static bool throw_on_range_error{true};

      using index_type = std::size_t;
      using partition_index_type = std::size_t;
      using partitions_type = maths::monotonic_sequence<partition_index_type, std::greater<partition_index_type>>;
      using partitions_allocator_type = typename partitions_type::allocator_type;

      template<class S> using container_type = std::vector<S, std::allocator<S>>;
    };

    template<class T, class Handler=object::by_value<T>, class Traits=partitioned_sequence_traits<T, Handler>>
      requires object::handler<Handler>
    class partitioned_sequence : public partitioned_sequence_base<T, Handler, Traits>
    {
    private:
      using base_t = partitioned_sequence_base<T, Handler, Traits>;
    public:
      using container_type            = typename partitioned_sequence_base<T, Handler, Traits>::container_type;
      using allocator_type            = typename container_type::allocator_type;
      using partitions_allocator_type = typename Traits::partitions_allocator_type;

      partitioned_sequence() = default;

      partitioned_sequence(const allocator_type& allocator, const partitions_allocator_type& partitionAllocator) noexcept
        : partitioned_sequence_base<T, Handler, Traits>(allocator, partitionAllocator)
      {}

      partitioned_sequence(std::initializer_list<std::initializer_list<T>> list, const allocator_type& allocator=allocator_type{}, const partitions_allocator_type& partitionAllocator=partitions_allocator_type{})
        : partitioned_sequence_base<T, Handler, Traits>(list, allocator, partitionAllocator)
      {}

      partitioned_sequence(const partitioned_sequence&) = default;

      partitioned_sequence(const partitioned_sequence& s, const allocator_type& allocator, const partitions_allocator_type& partitionAllocator)
        : partitioned_sequence_base<T, Handler, Traits>(s, allocator, partitionAllocator)
      {}

      partitioned_sequence(partitioned_sequence&&) noexcept = default;

      partitioned_sequence(partitioned_sequence&& s, const allocator_type& allocator, const partitions_allocator_type& partitionAllocator)
        : partitioned_sequence_base<T, Handler, Traits>(std::move(s), allocator, partitionAllocator)
      {}

      ~partitioned_sequence() = default;

      partitioned_sequence& operator=(const partitioned_sequence&) = default;
      partitioned_sequence& operator=(partitioned_sequence&&)      = default;

      void swap(partitioned_sequence& other)
        noexcept(noexcept(this->base_t::swap(other)))
      {
        base_t::swap(other);
      }

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

    template<class T, std::size_t Npartitions, std::size_t Nelements, std::integral IndexType>
    struct static_partitioned_sequence_traits
    {
      constexpr static bool static_storage_v{true};
      constexpr static bool throw_on_range_error{true};
      constexpr static std::size_t num_partitions_v{Npartitions};
      constexpr static std::size_t num_elements_v{Nelements};

      using index_type = IndexType;
      using partition_index_type = std::make_unsigned_t<IndexType>;
      using partitions_type = maths::static_monotonic_sequence<partition_index_type, Npartitions, std::greater<partition_index_type>>;

      template<class S> using container_type = std::array<S, Nelements>;

      constexpr static auto make_partitions(std::initializer_list<std::initializer_list<T>> list)
      {
        constexpr auto N{num_partitions_v};
        constexpr auto upper{std::numeric_limits<partition_index_type>::max()};
        const auto fn{
          [](const auto& l) {
            if (l.size() > upper)
              throw std::out_of_range{"Partition size out of bounds"};

            return static_cast<partition_index_type>(l.size());
          }
        };

        auto sizes{utilities::to_array<partition_index_type, N>(list, fn)};
        if(!sizes.empty())
        {
          for(auto iter{sizes.begin()+1}; iter!=sizes.end(); ++iter)
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

    template<class T, std::size_t Npartitions, std::size_t Nelements, std::integral IndexType=std::size_t>
    class static_partitioned_sequence :
      public partitioned_sequence_base<T, object::by_value<T>, static_partitioned_sequence_traits<T, Npartitions, Nelements,IndexType>>
    {
    public:
      using partitioned_sequence_base<
        T,
        object::by_value<T>,
        static_partitioned_sequence_traits<T, Npartitions, Nelements,IndexType>
      >::partitioned_sequence_base;
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

  template<class PartitionedData, class Pred, std::integral Index=typename PartitionedData::index_type>
  void erase_from_partition_if(PartitionedData& data, const Index partitionIndex, Pred pred)
  {
    auto i{data.begin_partition(partitionIndex)};
    while(i != data.end_partition(partitionIndex))
    {
      i = pred(*i) ? data.erase_from_partition(i) : std::next(i);
    }
  }
}
