////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/ContainerUtilities/IteratorDetails.hpp"

#include <iterator>

/*! \file
    \brief Implementation for an iterator with policies controlling dereferencing and auxiliary data.
 */

namespace sequoia::utilities
{
  struct null_data_policy
  {
  protected:
    constexpr null_data_policy() = default;

    constexpr null_data_policy(const null_data_policy&)     = default;
    constexpr null_data_policy(null_data_policy&&) noexcept = default;

    ~null_data_policy() = default;

    constexpr null_data_policy& operator=(const null_data_policy&)     = default;
    constexpr null_data_policy& operator=(null_data_policy&&) noexcept = default;

    [[nodiscard]]
    friend constexpr bool operator==(const null_data_policy&, const null_data_policy&) noexcept = default;
  };

  template<std::input_or_output_iterator Iterator, class AuxiliaryDataPolicy=null_data_policy>
  struct identity_dereference_policy : public AuxiliaryDataPolicy
  {
    using base_iterator_type    = Iterator;
    using auxiliary_data_policy = AuxiliaryDataPolicy;

    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using reference  = typename std::iterator_traits<Iterator>::reference;
    using pointer    = typename std::iterator_traits<Iterator>::pointer;


    template<class... Args>
      requires (!resolve_to_copy_v<identity_dereference_policy, Args...>)
    constexpr explicit(sizeof...(Args) == 1) identity_dereference_policy(Args&&... args)
      : AuxiliaryDataPolicy{std::forward<Args>(args)...}
    {}

    constexpr identity_dereference_policy(const identity_dereference_policy&) = default;

    [[nodiscard]]
    constexpr static reference get(reference ref) noexcept { return ref; }

    [[nodiscard]]
    constexpr static pointer get_ptr(reference ref) noexcept { return &ref; }

    [[nodiscard]]
    friend constexpr bool operator==(const identity_dereference_policy&, const identity_dereference_policy&) noexcept = default;
  protected:
    constexpr identity_dereference_policy(identity_dereference_policy&&) noexcept = default;

    ~identity_dereference_policy() = default;

    constexpr identity_dereference_policy& operator=(const identity_dereference_policy&)     = default;
    constexpr identity_dereference_policy& operator=(identity_dereference_policy&&) noexcept = default;
  };

  /*! \class iterator
      \brief An iterator with policies controlling dereferencing and auxiliary data.

      The DereferencePolicy allows customisation of the various dereferencing operators. In principle
      it is therefore possible that deferencing will not return a reference. This is forbidden
      in the case that the underlying iterator is non-const since the semantics could then
      be rather confusing. However, for const iterators this is tacitly allowed.
   */

  template<std::input_or_output_iterator Iterator, dereference_policy DereferencePolicy>
  class iterator : public DereferencePolicy
  {
  public:
    using dereference_policy = DereferencePolicy;
    using base_iterator_type = Iterator;

    using iterator_category      = typename std::iterator_traits<Iterator>::iterator_category;
    using difference_type        = typename std::iterator_traits<Iterator>::difference_type;
    using value_type             = typename DereferencePolicy::value_type;
    using pointer                = typename DereferencePolicy::pointer;
    using reference              = typename DereferencePolicy::reference;
    using const_dereference_type = impl::type_generator_t<DereferencePolicy>;

    constexpr iterator() = default;

    template<class Arg, class... Args>
      requires (!resolve_to_copy_v<iterator, Arg, Args...>)
    constexpr explicit(sizeof...(Args) == 0) iterator(Arg&& baseIterArg, Args&&... args)
      : DereferencePolicy{std::forward<Args>(args)...}
      , m_BaseIterator{std::forward<Arg>(baseIterArg)}
    {}

    template<std::input_or_output_iterator Iter, class DerefPol>
      requires (   !std::is_same_v<Iter, Iterator>
                && std::is_convertible_v<Iter, base_iterator_type>
                && impl::are_compatible_v<DereferencePolicy, DerefPol>
                && std::is_same_v<
                     std::remove_cvref_t<impl::type_generator_t<DereferencePolicy>>,
                     std::remove_cvref_t<impl::type_generator_t<DerefPol>>>)
    constexpr iterator(iterator<Iter, DerefPol> iter)
      : DereferencePolicy{static_cast<DerefPol&>(iter)}
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
    constexpr const_dereference_type operator*() const
      requires (!impl::provides_mutable_reference_v<DereferencePolicy>)
    {
      return DereferencePolicy::get(*m_BaseIterator);
    }

    [[nodiscard]]
    constexpr reference operator*() const
      requires impl::provides_mutable_reference_v<DereferencePolicy>
    {
      return DereferencePolicy::get(*m_BaseIterator);
    }

    [[nodiscard]]
    constexpr const_dereference_type operator[](const difference_type n) const
    {
      return DereferencePolicy::get(m_BaseIterator[n]);
    }

    [[nodiscard]]
    constexpr reference operator[](const difference_type n)
      requires impl::provides_mutable_reference_v<DereferencePolicy>
    {
      return DereferencePolicy::get(m_BaseIterator[n]);
    }

    constexpr std::conditional_t<is_const_pointer_v<pointer>, pointer, const pointer> operator->() const
    {
      return DereferencePolicy::get_ptr(*m_BaseIterator);
    }

    constexpr pointer operator->()
      requires is_const_pointer_v<pointer>
    {
      return DereferencePolicy::get_ptr(*m_BaseIterator);
    }

    constexpr iterator& operator++() { std::ranges::advance(m_BaseIterator, 1); return *this; }

    constexpr iterator& operator+=(const difference_type n) { std::ranges::advance(m_BaseIterator, n); return *this; }

    [[nodiscard]]
    friend constexpr iterator operator+(const iterator& it, const difference_type n)
    {
      iterator tmp{it};
      return tmp+=n;
    }

    constexpr iterator operator++(int)
    {
      iterator tmp{*this};
      operator++();
      return tmp;
    }

    constexpr iterator& operator--() { std::ranges::advance(m_BaseIterator, -1); return *this; }

    constexpr iterator& operator-=(const difference_type n) { std::ranges::advance(m_BaseIterator, -n); return *this; }

    [[nodiscard]]
    friend constexpr iterator operator-(const iterator& it, const difference_type n)
    {
      iterator tmp{it};
      return tmp-=n;
    }

    [[nodiscard]]
    friend constexpr difference_type operator-(const iterator& i, const iterator& j)
    {
      return i.base_iterator() - j.base_iterator();
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
      return std::ranges::distance(lhs.m_BaseIterator, rhs.m_BaseIterator);
    }
  private:
    Iterator m_BaseIterator;
  };
}
