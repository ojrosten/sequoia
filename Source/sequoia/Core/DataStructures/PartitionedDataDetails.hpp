////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file PartitionedDataDetails.hpp
    \brief Metaprogramming components for partitioned data.
 */

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <ranges>
#include <vector>

namespace sequoia::data_structures::partition_impl
{
  template<bool Reversed, std::integral IndexType>
  class partition_index_policy
  {
  public:
    using index_type = IndexType;

    constexpr partition_index_policy() = default;
    constexpr explicit partition_index_policy(const index_type n) : m_Partition{n} {}
    constexpr partition_index_policy(const partition_index_policy&) = default;

    constexpr static auto npos{std::numeric_limits<index_type>::max()};

    [[nodiscard]]
    constexpr static bool reversed() noexcept { return Reversed; }

    [[nodiscard]]
    friend constexpr bool operator==(const partition_index_policy& lhs, const partition_index_policy& rhs) noexcept = default;

    [[nodiscard]]
    constexpr IndexType partition_index() const noexcept { return m_Partition; }
  protected:
    constexpr partition_index_policy(partition_index_policy&&) noexcept = default;

    ~partition_index_policy() = default;

    constexpr partition_index_policy& operator=(const partition_index_policy&)     = default;
    constexpr partition_index_policy& operator=(partition_index_policy&&) noexcept = default;
  private:
    IndexType m_Partition{};
  };

  //============================= Helper classes for copy constructor ===================================//

  template<alloc PartitionAllocator, alloc Allocator>
  inline constexpr bool is_always_equal_v{
       std::allocator_traits<PartitionAllocator>::is_always_equal::value
    && std::allocator_traits<Allocator>::is_always_equal::value
  };

  template<alloc PartitionAllocator, alloc Allocator>
  inline constexpr bool propagates_on_copy_assignment_v{
       (    std::allocator_traits<PartitionAllocator>::propagate_on_container_copy_assignment::value
         || std::allocator_traits<PartitionAllocator>::is_always_equal::value)
    && (    std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value
         || std::allocator_traits<Allocator>::is_always_equal::value)
  };

  template<alloc PartitionAllocator, alloc Allocator>
  inline constexpr bool propagates_on_move_assignment_v{
       (    std::allocator_traits<PartitionAllocator>::propagate_on_container_move_assignment::value
         || std::allocator_traits<PartitionAllocator>::is_always_equal::value)
    && (    std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
         || std::allocator_traits<Allocator>::is_always_equal::value)
  };

  //============================= Helper for constraining the partitions ===================================//

  template<class Partitions>
  inline constexpr bool translatable_bounds_v{
    requires(Partitions& partitions, const std::ptrdiff_t delta) {
      partitions.translate(partitions.cbegin(), partitions.cend(), delta);
    }
  };

  template<class Partitions>
  inline constexpr bool overwritable_bounds_v{
    requires(Partitions& partitions, const std::vector<typename Partitions::value_type>& bounds) {
      partitions.overwrite(bounds);
    }
  };

  /** \brief Whether `Partitions` admits a bound equal to, or above, the one
             before it.

      A partition's bound is the number of elements up to its end, so a
      later bound is never below an earlier one.
   */
  template<class Partitions>
  inline constexpr bool admits_non_decreasing_bounds_v{
    requires {
      typename Partitions::compare_type;
      requires (   !typename Partitions::compare_type{}(typename Partitions::value_type{},
                                                        typename Partitions::value_type{})
                && !typename Partitions::compare_type{}(typename Partitions::value_type{},
                                                        typename Partitions::value_type{1}));
    }
  };

  //============================= Helper for construction from nested ranges ===================================//

  template<class Range>
  using const_inner_range_t = const std::remove_reference_t<std::ranges::range_reference_t<const Range>>;

  template<class Range, class T>
  inline constexpr bool nested_sized_range_of_v{
    requires {
      requires std::ranges::sized_range<const Range>;
      requires std::ranges::sized_range<const_inner_range_t<Range>>;
      requires std::same_as<std::ranges::range_value_t<const_inner_range_t<Range>>, T>;
    }
  };
}
