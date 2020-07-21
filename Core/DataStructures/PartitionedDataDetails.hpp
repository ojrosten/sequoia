////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file PartitionedDataDetails.hpp
    \brief Metaprogramming components for partitioned data.
 */

#include "Handlers.hpp"

#include <map>

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

  template<class Traits, handler Handler> struct storage_type_generator
  {
    using held_type      = typename Handler::handle_type;
    using container_type = typename Traits::template container_type<held_type>;
  };
    
  template<class Traits, handler Handler, template<class> class ReferencePolicy, bool Reversed>
  struct partition_iterator_generator
  {
    using iterator = typename storage_type_generator<Traits, Handler>::container_type::iterator;
  };

  template<class Traits, handler Handler>
  struct partition_iterator_generator<Traits, Handler, mutable_reference, true>
  {
    using iterator = typename storage_type_generator<Traits,Handler>::container_type::reverse_iterator;
  };
    
  template<class Traits, handler Handler>
  struct partition_iterator_generator<Traits, Handler, const_reference, false>
  {
    using iterator = typename storage_type_generator<Traits, Handler>::container_type::const_iterator;
  };

  template<class Traits, handler Handler>
  struct partition_iterator_generator<Traits, Handler, const_reference, true>
  {
    using iterator = typename storage_type_generator<Traits, Handler>::container_type::const_reverse_iterator;
  };
  
  template<bool Reversed, class IndexType>
  class partition_index_policy
  {
  public:
    using index_type = IndexType;

    constexpr explicit partition_index_policy(const index_type n) : m_Partition{n} {}
    constexpr partition_index_policy(const partition_index_policy&) = default;

    constexpr static auto npos{std::numeric_limits<index_type>::max()};

    [[nodiscard]]
    constexpr static bool reversed() noexcept { return Reversed; }

    [[nodiscard]]
    friend constexpr bool operator==(const partition_index_policy& lhs, const partition_index_policy& rhs) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator!=(const partition_index_policy& lhs, const partition_index_policy& rhs) noexcept = default;

    [[nodiscard]]
    constexpr IndexType partition_index() const noexcept { return m_Partition; }
  protected:          
    constexpr partition_index_policy(partition_index_policy&&) noexcept = default;
    
    ~partition_index_policy() = default;
    
    constexpr partition_index_policy& operator=(const partition_index_policy&)     = default;
    constexpr partition_index_policy& operator=(partition_index_policy&&) noexcept = default;
  private:
    IndexType m_Partition;
  };

  template<
    handler Handler,
    template<class> class ReferencePolicy,
    class AuxiliaryDataPolicy
  >
  struct dereference_policy : public Handler, public AuxiliaryDataPolicy
  {
    using elementary_type = typename Handler::elementary_type;
    using value_type      = elementary_type;
    using reference       = typename ReferencePolicy<elementary_type>::reference;
    using pointer         = typename ReferencePolicy<elementary_type>::pointer;

    template<class... Args>
      requires (!resolve_to_copy_v<dereference_policy, Args...>)
    constexpr dereference_policy(Args&&... args) : AuxiliaryDataPolicy{std::forward<Args>(args)...} {}
    
    constexpr dereference_policy(const dereference_policy&) = default;    
  protected:
    constexpr dereference_policy(dereference_policy&&) noexcept = default;

    ~dereference_policy() = default;

    constexpr dereference_policy& operator=(const dereference_policy&) = default;
    constexpr dereference_policy& operator=(dereference_policy&&) noexcept = default;
  };

  //============================= Helper classes for copy constructor ===================================//

  template<bool Direct>
  struct copy_constant : std::bool_constant<Direct>
  {};

  using direct_copy_type   = copy_constant<true>;
  using indirect_copy_type = copy_constant<false>;

  template<class PartitionAllocator, class Allocator>
  constexpr bool is_always_equal_v{
       std::allocator_traits<PartitionAllocator>::is_always_equal::value
    && std::allocator_traits<Allocator>::is_always_equal::value
  };

  template<class PartitionAllocator, class Allocator>
  constexpr bool propagates_on_copy_assignment_v{
       (    std::allocator_traits<PartitionAllocator>::propagate_on_container_copy_assignment::value
         || std::allocator_traits<PartitionAllocator>::is_always_equal::value)
    && (    std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value
         || std::allocator_traits<Allocator>::is_always_equal::value)
  };

  template<class PartitionAllocator, class Allocator>
  constexpr bool propagates_on_move_assignment_v{
       (    std::allocator_traits<PartitionAllocator>::propagate_on_container_move_assignment::value
         || std::allocator_traits<PartitionAllocator>::is_always_equal::value)
    && (    std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value
         || std::allocator_traits<Allocator>::is_always_equal::value)
  };

  template<handler Handler, class T>
  constexpr static bool direct_copy_v{std::is_same_v<Handler, ownership::independent<T>>};
  
  template<class T> class data_duplicator;
    
  template<class T> class data_duplicator<ownership::independent<T>>
  {
  public:
    [[nodiscard]]
    constexpr T duplicate(const T& in) const { return T{in}; }
  private:
  };

  template<class T> class data_duplicator<ownership::shared<T>>
  {
  public:
    using handle_type = typename ownership::shared<T>::handle_type;

    [[nodiscard]]
    handle_type duplicate(const handle_type in)
    {
      handle_type ptr{};
      auto found{m_ProcessedPointers.find(in)};
      if(found == m_ProcessedPointers.end())
      {
        ptr = ownership::shared<T>::make(*in);
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
}
