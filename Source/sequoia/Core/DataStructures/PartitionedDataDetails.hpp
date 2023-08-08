////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file PartitionedDataDetails.hpp
    \brief Metaprogramming components for partitioned data.
 */

#include "sequoia/Core/Object/Handlers.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <algorithm>
#include <vector>

namespace sequoia::data_structures::partition_impl
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

  template<class Traits, class Handler>
    requires object::handler<Handler>
  struct storage_type_generator
  {
    using held_type      = typename Handler::product_type;
    using container_type = typename Traits::template container_type<held_type>;
  };

  template<class Traits, class Handler, template<class> class ReferencePolicy, bool Reversed>
    requires object::handler<Handler>
  struct partition_iterator_generator
  {
    using iterator = typename storage_type_generator<Traits, Handler>::container_type::iterator;
  };

  template<class Traits, class Handler>
    requires object::handler<Handler>
  struct partition_iterator_generator<Traits, Handler, mutable_reference, true>
  {
    using iterator = typename storage_type_generator<Traits,Handler>::container_type::reverse_iterator;
  };

  template<class Traits, class Handler>
    requires object::handler<Handler>
  struct partition_iterator_generator<Traits, Handler, const_reference, false>
  {
    using iterator = typename storage_type_generator<Traits, Handler>::container_type::const_iterator;
  };

  template<class Traits, class Handler>
    requires object::handler<Handler>
  struct partition_iterator_generator<Traits, Handler, const_reference, true>
  {
    using iterator = typename storage_type_generator<Traits, Handler>::container_type::const_reverse_iterator;
  };

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

  template
  <
    class Handler,
    template<class> class ReferencePolicy,
    class AuxiliaryDataPolicy
  >
    requires object::handler<Handler>
  struct dereference_policy_for : protected Handler, public AuxiliaryDataPolicy
  {
    using value_type = typename Handler::value_type;
    using reference  = typename ReferencePolicy<value_type>::reference;
    using pointer    = typename ReferencePolicy<value_type>::pointer;

    template<class... Args>
      requires (!resolve_to_copy_v<dereference_policy_for, Args...>)
    constexpr dereference_policy_for(Args&&... args) : AuxiliaryDataPolicy{std::forward<Args>(args)...} {}

    constexpr dereference_policy_for(const dereference_policy_for&) = default;

    template<std::input_or_output_iterator I>
    [[nodiscard]]
    constexpr static reference get(I i) { return Handler::get(*i); }

    template<std::input_or_output_iterator I>
    [[nodiscard]]
    constexpr static pointer get_ptr(I i) { return Handler::get_ptr(*i); }
  protected:
    constexpr dereference_policy_for(dereference_policy_for&&) noexcept = default;

    ~dereference_policy_for() = default;

    constexpr dereference_policy_for& operator=(const dereference_policy_for&) = default;
    constexpr dereference_policy_for& operator=(dereference_policy_for&&) noexcept = default;
  };

  //============================= Helper classes for copy constructor ===================================//

  template<bool Direct>
  struct copy_constant : std::bool_constant<Direct>
  {};

  using direct_copy_type   = copy_constant<true>;
  using indirect_copy_type = copy_constant<false>;

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

  template<class Handler, class T>
    requires object::handler<Handler>
  inline constexpr bool direct_copy_v{std::is_same_v<Handler, object::by_value<T>>};

  template<class T> class data_duplicator;

  template<class T> class data_duplicator<object::by_value<T>>
  {
  public:
    [[nodiscard]]
    constexpr T duplicate(const T& in) const { return T{in}; }
  private:
  };

  template<class T>
  class data_duplicator<object::shared<T>>
  {
  public:
    using product_type = typename object::shared<T>::product_type;

    [[nodiscard]]
    product_type duplicate(const product_type in)
    {
      auto found{std::ranges::lower_bound(m_ProcessedPointers, in, std::ranges::less{}, [](const key_val& keyVal){ return keyVal.first; })};
      return ((found == m_ProcessedPointers.end()) || (found->first != in))
        ? m_ProcessedPointers.emplace(found, in, object::shared<T>::producer_type::make(*in))->second
        : found->second;
    }
  private:
    using key_val = std::pair<product_type, product_type>;
    std::vector<key_val> m_ProcessedPointers;
  };
}
