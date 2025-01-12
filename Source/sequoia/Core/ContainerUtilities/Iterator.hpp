////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "sequoia/Core/Meta/TypeTraits.hpp"

#include <iterator>

/*! \file
    \brief Implementation for an iterator with policies controlling dereferencing and auxiliary data.
 */

namespace sequoia::utilities
{
  template<class I>
  concept decrementable = requires(I i) {
    { --i } -> std::same_as<I&>;
    { i-- } -> std::convertible_to<I>;
  };

  template<class I>
  concept steppable = requires(I i, const I j, const std::iter_difference_t<I> n) {
    { i += n } -> std::same_as<I&>;
    { j + n }  -> std::convertible_to<I>;
    { n + j }  -> std::convertible_to<I>;
    { i -= n } -> std::same_as<I&>;
    { j - n }  -> std::convertible_to<I>;
  };

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

  /*! \brief Detects pointer_type */

  template<class Iterator, dereference_policy_for<Iterator> Deref>
  struct pointer_type
  {
    using type = void;
  };

  template<class Iterator, dereference_policy_for<Iterator> Deref>
    requires requires { typename Deref::pointer; }
  struct pointer_type<Iterator, Deref>
  {
    using type = typename Deref::pointer;
  };

  template<class Iterator, dereference_policy_for<Iterator> Deref>
  using pointer_type_t = typename pointer_type<Iterator, Deref>::type;

  /*! \brief Detects difference_type */

  template<class T>
  inline constexpr bool has_difference_type{requires { typename T::difference_type; }};

  template<class Iterator, dereference_policy_for<Iterator> Deref>
  struct difference_type;

  template<class Iterator, dereference_policy_for<Iterator> Deref>
    requires has_difference_type<Deref>
  struct difference_type<Iterator, Deref>
  {
    using type = typename Deref::difference_type;
  };

  template<class Iterator, dereference_policy_for<Iterator> Deref>
    requires (!has_difference_type<Deref> && std::input_or_output_iterator<Iterator>)
  struct difference_type<Iterator, Deref>
  {
    using type = typename std::iterator_traits<Iterator>::difference_type;
  };

  template<class Iterator, dereference_policy_for<Iterator> Deref>
  using difference_type_t = typename difference_type<Iterator, Deref>::type;

  /*! \brief Detects value_type */

  template<class T>
  struct value_type
  {
    using type = void;
  };

  template<class I>
    requires std::input_or_output_iterator<I>
  struct value_type<I>
  {
    using type = std::iter_value_t<I>;
  };

  template<class T>
    requires (!std::input_or_output_iterator<T> && has_value_type<T>)
  struct value_type<T>
  {
    using type = typename T::value_type;
  };

  template<class T>
  using value_type_t = typename value_type<T>::type;

  /*! \brief Detects reference_type */

  template<class T>
  inline constexpr bool has_reference_type{requires { typename T::reference; }};

  template<class T>
  struct reference_type
  {
    using type = void;
  };

  template<class I>
    requires std::input_or_output_iterator<I>
  struct reference_type<I>
  {
    using type = std::iter_reference_t<I>;
  };

  template<class T>
    requires (!std::input_or_output_iterator<T> && has_reference_type<T>)
  struct reference_type<T>
  {
    using type = typename T::reference;
  };

  template<class T>
  using reference_type_t = typename reference_type<T>::type;

  /*! \brief Policy representing absence of additional data carried by the`identity_dereference_policy` */

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
    constexpr static reference get(Iterator i) { return *i; }

    [[nodiscard]]
    constexpr static pointer get_ptr(Iterator i) { return &*i; }

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
       (    std::indirectly_writable<reference_type_t<Iterator>, value_type_t<Iterator>>
         && std::indirectly_writable<reference_type_t<DereferencePolicy>, value_type_t<DereferencePolicy>>)
    || (    !std::indirectly_writable<reference_type_t<Iterator>, value_type_t<Iterator>>
         && !std::indirectly_writable<reference_type_t<DereferencePolicy>, value_type_t<DereferencePolicy>>)
  };

  /*! \class iterator
      \brief An iterator with policies controlling dereferencing and auxiliary data.

      The DereferencePolicy allows customisation of the various dereferencing operators. In principle
      it is therefore possible that dereferencing will not return a reference, which can be useful.
      However, this can cause the semantics to be confusing if the underlying iterator is
      indirectly_writable. Therefore, this is forbidden.
   */

  template<class Iterator, dereference_policy_for<Iterator> DereferencePolicy>
    requires has_sensible_semantics<Iterator, DereferencePolicy>
  class iterator : public DereferencePolicy
  {
  public:
    using base_iterator_type      = Iterator;
    using dereference_policy_type = DereferencePolicy;

    using value_type      = typename DereferencePolicy::value_type;
    using reference       = typename DereferencePolicy::reference;
    using difference_type = difference_type_t<Iterator, DereferencePolicy>;
    using pointer         = pointer_type_t<Iterator, DereferencePolicy>;

    constexpr iterator() = default;

    template<class Arg, class... Args>
      requires (!resolve_to_copy_v<iterator, Arg, Args...>)
    constexpr explicit(sizeof...(Args) == 0) iterator(Arg&& baseIterArg, Args&&... args)
      : DereferencePolicy{std::forward<Args>(args)...}
      , m_BaseIterator{std::forward<Arg>(baseIterArg)}
    {}

    template<class Iter, class DerefPol>
      requires (   !std::is_same_v<Iter, Iterator>
                && initializable_from<Iterator, Iter>
                && consistent_policies_v<DereferencePolicy, DerefPol>)
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
      requires steppable<Iterator>
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

    constexpr iterator& operator+=(const difference_type n)
      requires steppable<Iterator>
    {
      m_BaseIterator += n;
      return *this;
    }

    [[nodiscard]]
    friend constexpr iterator operator+(const iterator& it, const difference_type n)
      requires steppable<Iterator>
    {
      iterator tmp{it};
      return tmp+=n;
    }

    [[nodiscard]]
    friend constexpr iterator operator+(const difference_type n, const iterator& it)
      requires steppable<Iterator>
    {
      return it + n;
    }

    constexpr iterator& operator--()
      requires decrementable<Iterator>
    {
      --m_BaseIterator; return *this;
    }

    constexpr iterator operator--(int)
      requires decrementable<Iterator>
    {
      iterator tmp{*this};
      operator--();
      return tmp;
    }

    constexpr iterator& operator-=(const difference_type n)
      requires steppable<Iterator>
    {
      m_BaseIterator -=n;
      return *this;
    }

    [[nodiscard]]
    friend constexpr iterator operator-(const iterator& it, const difference_type n)
      requires steppable<Iterator>
    {
      iterator tmp{it};
      return tmp-=n;
    }

    [[nodiscard]]
    friend constexpr difference_type operator-(const iterator& lhs, const iterator& rhs)
      requires steppable<Iterator>
    {
      return lhs.base_iterator() - rhs.base_iterator();
    }

    [[nodiscard]]
    friend constexpr bool operator==(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator == rhs.m_BaseIterator;
    }

    [[nodiscard]]
    friend constexpr auto operator<=>(const iterator& lhs, const iterator& rhs) noexcept
    {
      return lhs.m_BaseIterator <=> rhs.m_BaseIterator;
    }
  private:
    Iterator m_BaseIterator{};
  };
}
