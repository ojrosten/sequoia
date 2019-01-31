////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "TypeTraits.hpp"

#include <iterator>

/*! \file Iterator.hpp
    \brief Implementation for an iterator with policies controlling dereferencing and auxiliary data.
 */

namespace sequoia::utilities
{
  template<class Iterator>
  struct identity_dereference_policy
  {
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using reference = typename std::iterator_traits<Iterator>::reference;
    using pointer = typename std::iterator_traits<Iterator>::pointer;

    [[nodiscard]]
    static constexpr reference get(reference ref) noexcept { return ref; }

  protected:
    constexpr identity_dereference_policy() = default;
    constexpr identity_dereference_policy(const identity_dereference_policy&)     = default;
    constexpr identity_dereference_policy(identity_dereference_policy&&) noexcept = default;
    
    ~identity_dereference_policy() = default;

    constexpr identity_dereference_policy& operator=(const identity_dereference_policy&)     = default;
    constexpr identity_dereference_policy& operator=(identity_dereference_policy&&) noexcept = default;
  };

  struct null_data_policy
  {
  protected:
    constexpr null_data_policy() = default;
    ~null_data_policy() = default;
      
    constexpr null_data_policy(const null_data_policy&)                = default;
    constexpr null_data_policy(null_data_policy&&) noexcept            = default;
    constexpr null_data_policy& operator=(const null_data_policy&)     = default;
    constexpr null_data_policy& operator=(null_data_policy&&) noexcept = default;
  };

  [[nodiscard]]
  inline constexpr bool operator==(const null_data_policy& lhs, const null_data_policy& rhs) noexcept
  {
    return true;
  }

  [[nodiscard]]
  inline constexpr bool operator!=(const null_data_policy& lhs, const null_data_policy& rhs) noexcept
  {
    return !(lhs == rhs);
  }

  namespace impl
  {
    template<class DereferencePolicy, class=std::void_t<>>
    struct provides_reference_type : std::false_type
    {};

    template<class DereferencePolicy>
    struct provides_reference_type<DereferencePolicy, std::void_t<typename DereferencePolicy::reference>>
      : std::true_type
    {};
    
    template<class DereferencePolicy>
    constexpr bool provides_reference_type_v{provides_reference_type<DereferencePolicy>::value};

    
    template<class DereferencePolicy, class=std::void_t<>>
    struct provides_proxy_type : std::false_type
    {};

    template<class DereferencePolicy>
    struct provides_proxy_type<DereferencePolicy, std::void_t<typename DereferencePolicy::proxy>>
      : std::true_type
    {};
    
    template<class DereferencePolicy>
    constexpr bool provides_proxy_type_v{provides_proxy_type<DereferencePolicy>::value};

    template<class DereferencePolicy>
    constexpr bool is_valid_v{
         (provides_proxy_type_v<DereferencePolicy> && !provides_reference_type_v<DereferencePolicy>)
      || (!provides_proxy_type_v<DereferencePolicy> && provides_reference_type_v<DereferencePolicy>)
    };
    
    template<
      class DereferencePolicy,
      bool=provides_reference_type_v<DereferencePolicy>,
      bool=provides_proxy_type_v<DereferencePolicy>
    >
    struct type_generator {};       

    template<class DereferencePolicy> struct type_generator<DereferencePolicy, false, true>
    {
      using type = typename DereferencePolicy::proxy;
    };

    template<class DereferencePolicy>
    struct type_generator<DereferencePolicy, true, false>
    {
      using reference = typename DereferencePolicy::reference;
      using type = std::conditional_t<is_const_reference_v<reference>, reference, const reference>;
    };
  }

  /*! \class iterator
      \brief An iterator with policies controlling dereferncing and auxiliary data.

      The DereferencePolicy allows customisation of the various dereferencing operators. In principle
      it is therefore possible that deferencing will not return a reference. This is forbidden
      in the case that the underlying iterator is non-const since the semantics could then
      be rather confusing. However, for const iterators this is tacitly allowed.

      The iterator may
      also hold extra (auxiliary) data, controlled via the AuxiliaryDataPolicy.

   */

  template<class Iterator, class DereferencePolicy, class AuxiliaryDataPolicy>
  class iterator : public DereferencePolicy, public AuxiliaryDataPolicy
  {
  public:    
    using dereference_policy    = DereferencePolicy;
    using auxiliary_data_policy = AuxiliaryDataPolicy;
    using base_iterator_type    = Iterator;
      
    using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;
    using difference_type   = typename std::iterator_traits<Iterator>::difference_type;      
    using value_type        = typename DereferencePolicy::value_type;
    using pointer           = typename DereferencePolicy::pointer;

    // remove this
    using reference         = typename DereferencePolicy::reference;

    static_assert(impl::is_valid_v<DereferencePolicy>,
      "The DereferencePolicy must supply exacly one type called either reference or proxy");

    template<
      class Arg,
      class... Args,
      class=std::enable_if_t<sizeof...(Args) || !same_decay_v<Arg, iterator>>
     >
    constexpr explicit iterator(Arg&& baseIterArg, Args&&... args)
      : AuxiliaryDataPolicy{std::forward<Args>(args)...}
      , m_BaseIterator{std::forward<Arg>(baseIterArg)}
    {
    }

    // Needs fixing...
    template<
      class Iter,
      class DerefPol,
      class=std::enable_if_t<
           !std::is_same_v<Iter, Iterator>
        && std::is_convertible_v<Iter, base_iterator_type>
        && std::is_same_v<
              std::remove_cv_t<std::remove_reference_t<typename DerefPol::reference>>,
              std::remove_cv_t<std::remove_reference_t<reference>>
           >
      >
    >
    constexpr iterator(iterator<Iter, DerefPol, AuxiliaryDataPolicy> iter)
      : AuxiliaryDataPolicy{static_cast<AuxiliaryDataPolicy&>(iter)}
      , m_BaseIterator{iter.base_iterator()} 
    {
    }
      
    ~iterator() = default;
      
    constexpr iterator(const iterator&)                = default;
    constexpr iterator(iterator&&) noexcept            = default;
    constexpr iterator& operator=(const iterator&)     = default;
    constexpr iterator& operator=(iterator&&) noexcept = default;

    [[nodiscard]]
    constexpr base_iterator_type base_iterator() const noexcept { return m_BaseIterator; }

    [[nodiscard]]
    constexpr
    typename impl::type_generator<DereferencePolicy>::type
    operator*() const
    {
      return DereferencePolicy::get(*m_BaseIterator);
    }

    template<class Ref=reference, class=std::enable_if_t<!is_const_reference_v<Ref>>>
    [[nodiscard]]
    constexpr reference operator*()
    {
      return DereferencePolicy::get(*m_BaseIterator);
    }

    [[nodiscard]]
    constexpr
    typename impl::type_generator<DereferencePolicy>::type
    operator[](const difference_type n) const
    {
      return DereferencePolicy::get(m_BaseIterator[n]);
    }

    template<class Ref=reference, class=std::enable_if_t<!is_const_reference_v<Ref>>>
    [[nodiscard]]
    constexpr reference operator[](const difference_type n)
    {
      return DereferencePolicy::get(m_BaseIterator[n]);
    }

    constexpr std::conditional_t<is_const_pointer_v<pointer>, pointer, const pointer>
    operator->() const
    {
      return &DereferencePolicy::get(*m_BaseIterator);
    }

    template<class Ptr=pointer, class=std::enable_if_t<!is_const_pointer_v<Ptr>>>
    constexpr pointer operator->()
    {
      return &DereferencePolicy::get(*m_BaseIterator);
    }

    constexpr iterator& operator++() { ++m_BaseIterator; return *this; }

    constexpr iterator& operator+=(const difference_type n) { m_BaseIterator+=n; return *this; }

    [[nodiscard]]
    friend constexpr iterator operator+(const iterator& it, const difference_type n)
    {
      iterator tmp(it);
      return tmp+=n;
    }
      
    constexpr iterator operator++(int)
    {
      iterator tmp{*this};
      operator++();
      return tmp;
    }

    constexpr iterator& operator--() { --m_BaseIterator; return *this; }

    constexpr iterator& operator-=(const difference_type n) { m_BaseIterator-=n; return *this; }

    [[nodiscard]]
    friend constexpr iterator operator-(const iterator& it, const difference_type n)
    {
      iterator tmp{it};
      return tmp-=n;
    }
      
    constexpr iterator operator--(int)
    {
      iterator tmp{*this};
      operator--();
      return tmp;
    }

    [[nodiscard]]
    friend constexpr bool operator==(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator == rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr bool operator!=(const iterator& lhs, const iterator& rhs) noexcept
    {
      return !(lhs == rhs);
    }

    [[nodiscard]]
    friend constexpr bool operator>(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator > rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr bool operator<(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator < rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr bool operator<=(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator <= rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr bool operator>=(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator >= rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr auto distance(iterator lhs, iterator rhs)
    {
      return std::distance(lhs.m_BaseIterator, rhs.m_BaseIterator);
    }
  private:
    Iterator m_BaseIterator;
  };
}
