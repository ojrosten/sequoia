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

#include "SharingPolicies.hpp"
#include "TypeTraits.hpp"

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

  template<class Traits, class SharingPolicy> struct storage_type_generator
  {
    using held_type      = typename SharingPolicy::handle_type;
    using container_type = typename Traits::template container_type<held_type>;
  };
    
  template<class Traits, class SharingPolicy, template<class> class ReferencePolicy, bool Reversed>
  struct partition_iterator_generator
  {
    using iterator = typename storage_type_generator<Traits, SharingPolicy>::container_type::iterator;
  };

  template<class Traits, class SharingPolicy>
  struct partition_iterator_generator<Traits, SharingPolicy, mutable_reference, true>
  {
    using iterator = typename storage_type_generator<Traits,SharingPolicy>::container_type::reverse_iterator;
  };
    
  template<class Traits, class SharingPolicy>
  struct partition_iterator_generator<Traits, SharingPolicy, const_reference, false>
  {
    using iterator = typename storage_type_generator<Traits, SharingPolicy>::container_type::const_iterator;
  };

  template<class Traits, class SharingPolicy>
  struct partition_iterator_generator<Traits, SharingPolicy, const_reference, true>
  {
    using iterator = typename storage_type_generator<Traits, SharingPolicy>::container_type::const_reverse_iterator;
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
    friend constexpr bool operator==(const partition_index_policy& lhs, const partition_index_policy& rhs) noexcept
    {
      return lhs.m_Partition == rhs.m_Partition;
    }

    [[nodiscard]]
    friend constexpr bool operator!=(const partition_index_policy& lhs, const partition_index_policy& rhs) noexcept
    {
      return !(lhs == rhs);
    }

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
    class SharingPolicy,
    template<class> class ReferencePolicy,
    class AuxiliaryDataPolicy
  >
  struct dereference_policy : public SharingPolicy, public AuxiliaryDataPolicy
  {
    using elementary_type = typename SharingPolicy::elementary_type;
    using value_type      = elementary_type;
    using reference       = typename ReferencePolicy<elementary_type>::reference;
    using pointer         = typename ReferencePolicy<elementary_type>::pointer;

    template<
      class... Args,
      class=std::enable_if_t<!resolve_to_copy_constructor_v<dereference_policy, Args...>>
    >
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

  template<class SharingPolicy, class T>
  constexpr static bool direct_copy_v{std::is_same_v<SharingPolicy, data_sharing::independent<T>>};
  
  template<class T> class data_duplicator;
    
  template<class T> class data_duplicator<data_sharing::independent<T>>
  {
  public:
    [[nodiscard]]
    constexpr T duplicate(const T& in) const { return T{in}; }
  private:
  };

  template<class T> class data_duplicator<data_sharing::shared<T>>
  {
  public:
    using handle_type = typename data_sharing::shared<T>::handle_type;

    [[nodiscard]]
    handle_type duplicate(const handle_type in)
    {
      handle_type ptr{};
      auto found{m_ProcessedPointers.find(in)};
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
}
