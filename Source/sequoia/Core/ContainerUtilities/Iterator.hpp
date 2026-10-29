////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/PlatformSpecific/Macros.hpp"

#include <iterator>
#include <memory>
#include <type_traits>

/** \file
    \brief Implementation for an iterator with policies controlling dereferencing and auxiliary data.
 */

namespace sequoia::utilities
{
  namespace impl
  {
    template<class Policy>
    struct aggregator : Policy
    {
      using Policy::Policy;
      
      template<class I>
      decltype(auto) get(I i) const
        requires requires { static_cast<const Policy&>(*this).get(i); }
      {
        return Policy::get(i);
      }
    };

    /** \brief Whether a class deriving from `Policy` can initialize its
        `Policy` subobject from `Args`, as one of:
        -# A copy of the `Policy` subobject of the one argument;
        -# A `Policy`, or an `aggregator<Policy>`, initialized from `Args`.
     */
    template<class Policy, class... Args>
    inline constexpr bool base_initializable_from_v{
         ((sizeof...(Args) == 1) && (std::derived_from<std::remove_cvref_t<Args>, Policy> && ...))
      || initializable_from<Policy, Args...>
      // Admits a Policy whose destructor is protected
      || initializable_from<aggregator<Policy>, Args...>
    };
  }

  template<class Policy, class Iterator>
  concept dereference_policy_for = requires(impl::aggregator<Policy> agg, Iterator i) {
    typename Policy::value_type;
    typename Policy::reference;

    { agg.get(i) } -> std::same_as<typename Policy::reference>;
  };

  template<class Policy1, class Policy2>
  inline constexpr bool consistent_policies_v{
    initializable_from<impl::aggregator<Policy1>, impl::aggregator<Policy2>>
  };

  /** \brief Detects pointer_type */

  template<class Iterator, dereference_policy_for<Iterator> Deref>
  struct pointer_type
  {
    using type = void;
  };

  template<class Iterator, dereference_policy_for<Iterator> Deref>
    requires requires { typename Deref::pointer; }
  struct pointer_type<Iterator, Deref>
  {
    using type = Deref::pointer;
  };

  template<class Iterator, dereference_policy_for<Iterator> Deref>
  using pointer_type_t = pointer_type<Iterator, Deref>::type;

  /** \brief Policy representing absence of additional data carried by the`identity_dereference_policy` */

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

    using value_type = std::iter_value_t<Iterator>;
    using reference  = std::iter_reference_t<Iterator>;
    using pointer    = std::conditional_t<std::is_lvalue_reference_v<reference>, std::add_pointer_t<reference>, void>;

    template<class... Args>
      requires (!resolve_to_copy_v<identity_dereference_policy, Args...>)
            && impl::base_initializable_from_v<AuxiliaryDataPolicy, Args...>
    constexpr explicit(sizeof...(Args) == 1) identity_dereference_policy(Args&&... args)
      : AuxiliaryDataPolicy{std::forward<Args>(args)...}
    {}

    constexpr identity_dereference_policy(const identity_dereference_policy&) = default;

    [[nodiscard]]
    constexpr static reference get(const Iterator& i) { return *i; }

    [[nodiscard]]
    constexpr static pointer get_ptr(const Iterator& i)
      requires (!std::is_void_v<pointer>)
    {
      return std::addressof(*i);
    }

    [[nodiscard]]
    friend constexpr bool operator==(const identity_dereference_policy&, const identity_dereference_policy&) noexcept = default;
  protected:
    constexpr identity_dereference_policy(identity_dereference_policy&&) noexcept = default;

    ~identity_dereference_policy() = default;

    constexpr identity_dereference_policy& operator=(const identity_dereference_policy&)     = default;
    constexpr identity_dereference_policy& operator=(identity_dereference_policy&&) noexcept = default;
  };

  template<class Iterator, dereference_policy_for<Iterator> DereferencePolicy>
  inline constexpr bool has_sensible_semantics{
       (    std::indirectly_writable<std::iter_reference_t<Iterator>, std::iter_value_t<Iterator>>
         && std::indirectly_writable<typename DereferencePolicy::reference, typename DereferencePolicy::value_type>)
    || (    !std::indirectly_writable<std::iter_reference_t<Iterator>, std::iter_value_t<Iterator>>
         && !std::indirectly_writable<typename DereferencePolicy::reference, typename DereferencePolicy::value_type>)
  };

  namespace impl
  {
    /** \brief Whether an `iterator<FromIter, FromPolicy>` converts to an
        `iterator<ToIter, ToPolicy>`, through the converting constructor.
     */
    template<class FromIter, class FromPolicy, class ToIter, class ToPolicy>
    inline constexpr bool iterator_converts_v{
         !std::is_same_v<FromIter, ToIter>
      && initializable_from<ToIter, FromIter>
      && consistent_policies_v<ToPolicy, FromPolicy>
    };
  }

  /** \class iterator
      \brief An iterator with policies controlling dereferencing and auxiliary data.

      The DereferencePolicy allows customisation of the various dereferencing operators. In principle
      it is therefore possible that dereferencing will not return a reference, which can be useful.
      However, this can cause the semantics to be confusing if the underlying iterator is
      indirectly_writable. Therefore, this is forbidden.
   */

  template<std::input_or_output_iterator Iterator, dereference_policy_for<Iterator> DereferencePolicy>
    requires has_sensible_semantics<Iterator, DereferencePolicy>
  class iterator : public DereferencePolicy
  {
  private:
    /** Whether an `iterator<Iter, DerefPol>` converts to this iterator,
        through the converting constructor.
     */
    template<class Iter, class DerefPol>
    constexpr static bool converts_from_v{
      impl::iterator_converts_v<Iter, DerefPol, Iterator, DereferencePolicy>
    };

    /** Whether either of this iterator and an `iterator<Iter, DerefPol>`
        converts to the other.
     */
    template<class Iter, class DerefPol>
    constexpr static bool converts_either_way_v{
         converts_from_v<Iter, DerefPol>
      || impl::iterator_converts_v<Iterator, DereferencePolicy, Iter, DerefPol>
    };
  public:
    using base_iterator_type      = Iterator;
    using dereference_policy_type = DereferencePolicy;

    using value_type      = DereferencePolicy::value_type;
    using reference       = DereferencePolicy::reference;
    using difference_type = std::iter_difference_t<Iterator>;
    using pointer         = pointer_type_t<Iterator, DereferencePolicy>;

    constexpr iterator() = default;

    template<class Arg, class... Args>
      requires (!resolve_to_copy_v<iterator, Arg, Args...>)
    constexpr explicit(sizeof...(Args) == 0) iterator(Arg&& baseIterArg, Args&&... args)
      : DereferencePolicy{std::forward<Args>(args)...}
      , m_BaseIterator{std::forward<Arg>(baseIterArg)}
    {}

    template<class Iter, class DerefPol>
      requires converts_from_v<Iter, DerefPol>
    constexpr iterator(iterator<Iter, DerefPol> iter)
      : DereferencePolicy{static_cast<const DerefPol&>(iter)}
      , m_BaseIterator{iter.base_iterator()}
    {}

    ~iterator() = default;

    constexpr iterator(const iterator&)                = default;
    constexpr iterator(iterator&&) noexcept            = default;
    constexpr iterator& operator=(const iterator&)     = default;
    constexpr iterator& operator=(iterator&&) noexcept = default;

    [[nodiscard]]
    constexpr base_iterator_type base_iterator() const noexcept { return m_BaseIterator; }

    [[nodiscard]]
    constexpr reference operator*() const
    {
      return DereferencePolicy::get(m_BaseIterator);
    }

    [[nodiscard]]
    constexpr reference operator[](const difference_type n) const
      requires std::random_access_iterator<Iterator>
    {
      return DereferencePolicy::get(m_BaseIterator + n);
    }

    constexpr pointer operator->() const
      requires requires (Iterator i){ DereferencePolicy::get_ptr(i); }
    {
      return DereferencePolicy::get_ptr(m_BaseIterator);
    }

    constexpr iterator& operator++()
      requires std::weakly_incrementable<Iterator>
    {
      ++m_BaseIterator;
      return *this;
    }

    constexpr iterator operator++(int)
      requires std::weakly_incrementable<Iterator>
    {
      iterator tmp{*this};
      operator++();
      return tmp;
    }

    // -Warray-bounds fires here on ranges below libstdc++'s sort threshold: __final_insertion_sort
    // holds `__first + 16` inside `if(__last - __first > 16)`, and gcc evaluates that address for a
    // range which cannot reach it - a static graph has 2 edges. This wrapper knows no bounds, so
    // nothing here can be at fault.
SEQUOIA_GCC_SUPPRESS_BEGIN("-Warray-bounds")
    constexpr iterator& operator+=(const difference_type n)
      requires std::random_access_iterator<Iterator>
    {
      m_BaseIterator += n;
      return *this;
    }

    [[nodiscard]]
    friend constexpr iterator operator+(const iterator& it, const difference_type n)
      requires std::random_access_iterator<Iterator>
    {
      iterator tmp{it};
      return tmp += n;
    }
SEQUOIA_GCC_SUPPRESS_END

    [[nodiscard]]
    friend constexpr iterator operator+(const difference_type n, const iterator& it)
      requires std::random_access_iterator<Iterator>
    {
      return it + n;
    }

    constexpr iterator& operator--()
      requires std::bidirectional_iterator<Iterator>
    {
      --m_BaseIterator;
      return *this;
    }

    constexpr iterator operator--(int)
      requires std::bidirectional_iterator<Iterator>
    {
      iterator tmp{*this};
      operator--();
      return tmp;
    }

    constexpr iterator& operator-=(const difference_type n)
      requires std::random_access_iterator<Iterator>
    {
      m_BaseIterator -= n;
      return *this;
    }

    [[nodiscard]]
    friend constexpr iterator operator-(const iterator& it, const difference_type n)
      requires std::random_access_iterator<Iterator>
    {
      iterator tmp{it};
      return tmp -= n;
    }

    [[nodiscard]]
    friend constexpr difference_type operator-(const iterator& lhs, const iterator& rhs)
      requires std::sized_sentinel_for<Iterator, Iterator>
    {
      return lhs.base_iterator() - rhs.base_iterator();
    }

    [[nodiscard]]
    friend constexpr bool operator==(const iterator& lhs, const iterator& rhs) noexcept
      requires std::equality_comparable<Iterator>
    {
      return lhs.m_BaseIterator == rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr auto operator<=>(const iterator& lhs, const iterator& rhs) noexcept
      requires std::three_way_comparable<Iterator>
    {
      return lhs.m_BaseIterator <=> rhs.m_BaseIterator;
    }

    // Without these, comparing with an iterator that converts to this one is
    // ambiguous where the two iterators share a public base class declaring
    // the comparison. This iterator's own comparison then needs the
    // converting constructor for one operand, and the class's needs a
    // derived-to-base conversion for both, so neither is better.
    template<class Iter, class DerefPol>
      requires converts_from_v<Iter, DerefPol> && std::equality_comparable_with<Iterator, Iter>
    [[nodiscard]]
    friend constexpr bool operator==(const iterator& lhs, const iterator<Iter, DerefPol>& rhs) noexcept
    {
      return lhs.m_BaseIterator == rhs.base_iterator();
    }

    template<class Iter, class DerefPol>
      requires converts_from_v<Iter, DerefPol> && std::three_way_comparable_with<Iterator, Iter>
    [[nodiscard]]
    friend constexpr auto operator<=>(const iterator& lhs, const iterator<Iter, DerefPol>& rhs) noexcept
    {
      return lhs.m_BaseIterator <=> rhs.base_iterator();
    }

    // Without these deletions, a comparison that the base iterator lacks
    // would resolve to a comparison declared by the dereference policy, or by
    // a class from which the policy derives, through the conversion to a
    // public base class. Such a comparison ignores the base iterators.
    friend bool operator==(const iterator&, const iterator&)
      requires (!std::equality_comparable<Iterator>) = delete;

    friend auto operator<=>(const iterator&, const iterator&)
      requires (!std::three_way_comparable<Iterator>) = delete;

    // Without these deletions, comparing with an iterator that neither
    // converts to nor from this one would resolve to a comparison declared
    // by a public base class that the two dereference policies share. Such a
    // comparison ignores the base iterators.
    template<class Iter, class DerefPol>
      requires (!(converts_either_way_v<Iter, DerefPol> && std::equality_comparable_with<Iterator, Iter>))
    friend bool operator==(const iterator&, const iterator<Iter, DerefPol>&) = delete;

    template<class Iter, class DerefPol>
      requires (!(converts_either_way_v<Iter, DerefPol> && std::three_way_comparable_with<Iterator, Iter>))
    friend auto operator<=>(const iterator&, const iterator<Iter, DerefPol>&) = delete;
  private:
    Iterator m_BaseIterator{};
  };
}
